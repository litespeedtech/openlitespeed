/*
 * Copyright 2002-2018 Lite Speed Technologies Inc, All Rights Reserved.
 * LITE SPEED PROPRIETARY/CONFIDENTIAL.
 */

var EventEmitter = require('events').EventEmitter;
var os = require('os');
var fs = require('fs');
var http = require('http');
var util = require('util');
var net = require('net');

var socketObject = process.env.LSNODE_BIND_SOCKET
    ? { path: process.env.LSNODE_SOCKET }
    : { fd: 0 };

/*
 * Child process supervision.
 *
 * An application may create its own child processes.  When this process goes
 * away those children have to go away as well, otherwise they are re-parented
 * to init and stay around forever, holding memory, sockets and, on a restart,
 * possibly the listening socket of the application.
 *
 *   - every child created through child_process is tracked and signaled when
 *     this process terminates in a way it can observe: normal exit, exit()
 *     call, uncaught exception, SIGTERM/SIGINT/SIGHUP/SIGQUIT.
 *   - children created with fork() additionally load this file through
 *     --require.  That copy only installs a watcher which terminates the child
 *     as soon as its parent is gone.  It is the only thing that covers a
 *     SIGKILL of the parent, where no handler of ours can run, and it applies
 *     to grandchildren as well because the child tracks its own children too.
 *
 * Children started with spawn()/exec() cannot be given that watcher - they are
 * not necessarily node processes - so for them only the observable paths above
 * are handled.
 *
 * Set LSNODE_KEEP_CHILDREN=1 to disable all of this.
 */
var PPID_ENV = 'LSNODE_GUARD_PPID';
var PARENT_CHECK_INTERVAL = 1000;
var LISTEN_SOCKET_CHECK_INTERVAL = 1000;
var FORCE_KILL_DELAY = 3000;
var children = [];
var terminating = false;
var signalHandlers = null;
var signalDispatch = null;
var updatingSignals = false;
var signalUpdatePending = false;

if (require.main !== module) {
    // Loaded with --require into a child process, act as the watchdog only.
    // Read the parent pid first, supervising resets PPID_ENV to our own pid.
    var guardPpid = parseInt(process.env[PPID_ENV], 10);
    superviseChildProcesses();
    watchParent(guardPpid);
} else {
    module.isApplicationLoader = true;
    global.LsNode = new EventEmitter();
    superviseChildProcesses();
    startApplication();
}


function isKeepChildren() {
    var keep = process.env.LSNODE_KEEP_CHILDREN;
    return keep !== undefined && keep !== '' && keep !== '0';
}


function trackChild(child, detached) {
    if (!child || typeof child.on !== 'function') {
        return child;
    }
    for (var i = 0; i < children.length; i++) {
        if (children[i].child === child) {
            return child;
        }
    }
    var entry = { child: child, detached: detached === true };
    children.push(entry);
    updateSignalHandlers();
    function forget() {
        var idx = children.indexOf(entry);
        if (idx >= 0) {
            children.splice(idx, 1);
            updateSignalHandlers();
        }
    }
    child.on('exit', forget);
    // close also fires for a failed spawn that never emitted exit. Do not
    // install an error listener: that would swallow otherwise unhandled errors.
    child.on('close', forget);
    return child;
}


function updateSignalHandlers() {
    // Catching a signal in javascript replaces the default disposition of
    // dying immediately: the handler only runs once the event loop turns, so
    // a process stuck in a synchronous call or a long computation no longer
    // reacts to SIGTERM at all and the web server escalates to SIGKILL.
    // Only pay that price while there are children to clean up, and give the
    // default behavior back the moment the last child is gone.
    var signals = { SIGHUP: 1, SIGINT: 2, SIGQUIT: 3, SIGTERM: 15 };
    updatingSignals = true;
    try {
        if (children.length && !signalHandlers) {
            signalHandlers = {};
            Object.keys(signals).forEach(function(name) {
                signalHandlers[name] = function() {
                    // emit() snapshots the listeners BEFORE once/prependOnceListener
                    // wrappers remove themselves. A later signal gets a fresh snapshot.
                    var appOwned = signalDispatch && signalDispatch.name === name
                        ? signalDispatch.appOwned
                        : process.listenerCount(name) > 1;
                    if (appOwned) {
                        return;
                    }
                    // Exit with the conventional status; the exit hook signals children.
                    // This is an explicit exit, not restoration of the OS disposition.
                    process.exit(128 + signals[name]);
                };
            });
        }
        if (signalHandlers) {
            Object.keys(signalHandlers).forEach(function(name) {
                var handler = signalHandlers[name];
                var listeners = process.listeners(name);
                var installed = listeners.indexOf(handler) >= 0;
                var appOwned = listeners.some(function(listener) {
                    return listener !== handler;
                });
                // Bun's native signals can bypass process.emit. Avoid competing
                // with application handlers, including once/prependOnceListener.
                if (children.length && !appOwned) {
                    if (!installed) {
                        process.on(name, handler);
                    }
                } else if (installed) {
                    process.removeListener(name, handler);
                }
            });
            if (!children.length) {
                signalHandlers = null;
            }
        }
    } finally {
        updatingSignals = false;
    }
}


function observeSignalDispatch() {
    function listenersChanged(name) {
        if (updatingSignals || signalUpdatePending ||
            ['SIGHUP', 'SIGINT', 'SIGQUIT', 'SIGTERM'].indexOf(name) < 0) {
            return;
        }
        signalUpdatePending = true;
        // newListener runs before insertion. Defer changes until registration
        // or the current once-handler dispatch has completed.
        process.nextTick(function() {
            signalUpdatePending = false;
            updateSignalHandlers();
        });
    }
    process.on('newListener', listenersChanged);
    process.on('removeListener', listenersChanged);
    // A narrow wrapper on this process, not EventEmitter.prototype. Delegate
    // every event, argument, return value and exception to the previous emit.
    // This must remain in the process.emit chain; code that replaces emit
    // without delegating bypasses this observer and is not supported.
    var emit = process.emit;
    process.emit = function(name) {
        var handler = this === process && signalHandlers &&
            Object.prototype.hasOwnProperty.call(signalHandlers, name)
            ? signalHandlers[name] : null;
        if (!handler) {
            return emit.apply(this, arguments);
        }
        var listeners = process.listeners(name);
        var appOwned = false;
        for (var i = 0; i < listeners.length; i++) {
            if (listeners[i] !== handler) {
                appOwned = true;
                break;
            }
        }
        var previous = signalDispatch;
        signalDispatch = { name: name, appOwned: appOwned };
        try {
            return emit.apply(this, arguments);
        } finally {
            // Preserve the outer snapshot during nested synchronous emissions.
            signalDispatch = previous;
        }
    };
}


function readProcessTree() {
    // { ppid: [ pid, ... ] } of every process on the box, null when /proc is
    // not available.  Used to reach grandchildren, exec() for instance runs
    // the command under a shell and only that shell is a child of ours.
    var tree;
    try {
        var entries = fs.readdirSync('/proc');
        tree = {};
        for (var i = 0; i < entries.length; i++) {
            var name = entries[i];
            if (!/^[0-9]+$/.test(name)) {
                continue;
            }
            var ppid;
            try {
                var stat = fs.readFileSync('/proc/' + name + '/stat', 'ascii');
                // The command name is in parentheses and may itself contain
                // spaces and parentheses, ppid is the 2nd field after it.
                ppid = stat.substring(stat.lastIndexOf(')') + 2).split(' ')[1];
            } catch (err) {
                continue;       // Process went away while we were looking.
            }
            if (!ppid) {
                continue;
            }
            if (tree[ppid]) {
                tree[ppid].push(parseInt(name, 10));
            } else {
                tree[ppid] = [parseInt(name, 10)];
            }
        }
    } catch (err) {
        tree = null;
    }
    return tree;
}


function collectDescendants(pid, tree, found) {
    var kids = tree[String(pid)];
    for (var i = 0; kids && i < kids.length; i++) {
        if (found.indexOf(kids[i]) < 0) {
            found.push(kids[i]);
            collectDescendants(kids[i], tree, found);
        }
    }
    return found;
}


function getParentPid() {
    if (typeof process.ppid === 'number') {
        return process.ppid;
    }
    try {
        var stat = fs.readFileSync('/proc/self/stat', 'ascii');
        return parseInt(stat.substring(stat.lastIndexOf(')') + 2)
                        .split(' ')[1], 10);
    } catch (err) {
        return 0;
    }
}


function killChildren(signal) {
    // Called from a process 'exit' handler as well, must stay synchronous.
    var live = [];
    for (var i = 0; i < children.length; i++) {
        var child = children[i].child;
        if (child.pid && child.exitCode === null && child.signalCode === null) {
            live.push(children[i]);
        }
    }
    if (!live.length) {
        return;
    }

    var tree = readProcessTree();
    for (i = 0; i < live.length; i++) {
        var entry = live[i];
        var pids = tree ? collectDescendants(entry.child.pid, tree, []) : [];
        try {
            if (entry.detached) {
                // Own process group leader, take the whole group with it.
                process.kill(-entry.child.pid, signal);
            } else {
                entry.child.kill(signal);
            }
        } catch (err) {
            // Already gone or not ours anymore, nothing to do.
        }
        for (var j = 0; j < pids.length; j++) {
            try {
                process.kill(pids[j], signal);
            } catch (err) {
            }
        }
    }
}


function findOptions(args, from) {
    for (var i = from; i < args.length; i++) {
        if (args[i] && typeof args[i] === 'object' && !Array.isArray(args[i])) {
            return i;
        }
    }
    return -1;
}


function isDetached(args, from) {
    var idx = findOptions(args, from);
    return idx >= 0 && args[idx].detached === true;
}


function canPreload(execPath) {
    // The watchdog is preloaded with --require, which node understands and bun
    // takes as an alias of its own --preload.  A child that runs the binary we
    // are running always gets it, no guessing involved.  fork() may name some
    // other runtime through execPath; that one is judged by name, after
    // resolving symlinks, so a versioned install reached through something
    // like /opt/runtime/current is still recognized.  Anything left is started
    // untouched: losing the watchdog only costs a child its parent-death
    // check, while a flag an unknown runtime rejects costs it startup.
    if (!execPath || execPath === process.execPath) {
        return true;
    }
    var resolved = execPath;
    try {
        resolved = fs.realpathSync(execPath);
    } catch (err) {
    }
    return /^(node|bun)/.test(resolved.substring(resolved.lastIndexOf('/') + 1));
}


function superviseChildProcesses() {
    if (isKeepChildren()) {
        return;
    }

    observeSignalDispatch();

    var cp = require('child_process');
    var origSpawn = cp.spawn;
    var origFork = cp.fork;
    var origExec = cp.exec;
    var origExecFile = cp.execFile;

    // Inherited by every child unless it is given an environment of its own.
    process.env[PPID_ENV] = String(process.pid);

    cp.spawn = function(command) {
        return trackChild(origSpawn.apply(this, arguments),
                          isDetached(arguments, 1));
    };

    cp.execFile = function(file) {
        return trackChild(origExecFile.apply(this, arguments),
                          isDetached(arguments, 1));
    };

    // Some Node versions implement exec() through the exported execFile(),
    // while others call an internal function.  trackChild() de-duplicates the
    // former case.
    cp.exec = function(command) {
        return trackChild(origExec.apply(this, arguments),
                          isDetached(arguments, 1));
    };

    // execSync()/spawnSync() have been reaped by the time they return, so
    // fork() is the last one left.  Normalize its two valid signatures before
    // adding the preloader: fork(module[, options]) and
    // fork(module[, args][, options]).
    cp.fork = function(modulePath, args, options) {
        if (args === undefined || args === null) {
            args = [];
        } else if (typeof args === 'object' && !Array.isArray(args)) {
            options = args;
            args = [];
        }
        if (options === undefined || options === null) {
            options = {};
        }
        if (typeof options !== 'object' || Array.isArray(options)) {
            // Preserve Node's normal argument validation and error.
            return origFork.call(this, modulePath, args, options);
        }
        var opts = Object.assign({}, options);

        var execArgv = opts.execArgv || process.execArgv || [];
        if (canPreload(opts.execPath) && execArgv.indexOf(__filename) < 0) {
            opts.execArgv = execArgv.concat(['--require', __filename]);
        }
        if (opts.env) {
            // A private environment was given, our own copy of PPID_ENV would
            // not be inherited, put it in explicitly.
            opts.env = Object.assign({}, opts.env);
            opts.env[PPID_ENV] = String(process.pid);
        }
        return trackChild(origFork.call(this, modulePath, args, opts),
                          opts.detached === true);
    };

    process.on('exit', function() {
        killChildren('SIGTERM');
    });
    // Signal handlers are installed by updateSignalHandlers() when the first
    // child appears and removed again with the last one, see there for why.
}


function watchParent(ppid) {
    if (isKeepChildren() || !ppid) {
        return;
    }
    // The recorded pid is only meaningful in the parent's own pid namespace.
    // When getppid() matches it, ppid-watching is trustworthy: any later
    // change means the parent went away.  When it does not match at startup
    // the situation is ambiguous -- an intermediate process or a namespace
    // boundary (legitimate, keep running), OR the real parent already died
    // and a subreaper adopted us (must terminate).  A plain pid comparison
    // cannot tell these apart, and adopting the observed pid as the new
    // reference would silently accept a subreaper as our parent.
    //
    // The fork() IPC channel resolves the ambiguity: it stays open while the
    // real parent lives and closes ('disconnect') when it dies or calls
    // disconnect().  This watchdog is only preloaded into fork() children, so
    // the channel is normally present. We therefore trust ppid-watching only
    // when the baseline matches, and otherwise rely on the channel. PID 1 can
    // be a live container parent, and PID 0 can mean an invisible parent in
    // another namespace; neither alone proves that the parent died.
    var current = getParentPid();
    var trustPpid = (current === ppid);
    var timer;
    function checkParent() {
        var currentPpid = getParentPid();
        var gone = trustPpid
            ? (currentPpid && currentPpid !== ppid)
            : (process.connected !== true);
        if (!gone) {
            return;
        }
        if (timer) {
            clearInterval(timer);
        }
        process.removeListener('disconnect', checkParent);
        terminateSelf();
    }
    // With a matching PPID, deliberate IPC disconnect is harmless and polling
    // still detects parent death. With an ambiguous PPID, IPC must remain open:
    // absent/already closed IPC is not proof of a live parent. This deliberately
    // requires keeping IPC open in ambiguous setups.
    process.on('disconnect', checkParent);
    timer = setInterval(checkParent, PARENT_CHECK_INTERVAL);
    if (typeof timer.unref === 'function') {
        timer.unref();
    }
    checkParent();
}


function terminateSelf() {
    if (terminating) {
        return;
    }
    terminating = true;
    killChildren('SIGTERM');
    try {
        // Gives the application a chance to shut itself down cleanly, when it
        // has no handler of its own this exits immediately.
        process.kill(process.pid, 'SIGTERM');
    } catch (err) {
    }
    setTimeout(function() {
        killChildren('SIGKILL');
        process.exit(0);
    }, FORCE_KILL_DELAY);
}


function watchListeningSocket(path, boundSelf) {
    // A Unix-domain listener remains open after its directory entry is
    // unlinked.  A replacement listener also gets a different inode.  In both
    // cases this process is no longer the one LiteSpeed reaches by path, so do
    // not leave it serving inherited connections or consuming resources.
    var noWatch = process.env.LSNODE_NO_SOCKET_WATCH;
    if (noWatch !== undefined && noWatch !== '' && noWatch !== '0') {
        return;
    }
    var expected;
    try {
        expected = fs.lstatSync(path);
    } catch (err) {
        // In app-bind mode the socket was just created at this path, a
        // failure here is real.  With an inherited listener the path belongs
        // to the web server and may not be visible from this process at all
        // (a different mount namespace, for one); that is not a reason to
        // die, it only means nothing useful can be watched.
        if (boundSelf) {
            terminateSelf();
        }
        return;
    }
    if (!expected.isSocket()) {
        if (boundSelf) {
            terminateSelf();
        }
        return;
    }
    var timer = setInterval(function() {
        var current;
        try {
            current = fs.lstatSync(path);
        } catch (err) {
            // Permission changes and transient I/O failures do not establish
            // that the listener was removed. Retry those on the next tick.
            if (err.code !== 'ENOENT' && err.code !== 'ENOTDIR') {
                return;
            }
            clearInterval(timer);
            terminateSelf();
            return;
        }
        if (!current.isSocket() || current.dev !== expected.dev ||
            current.ino !== expected.ino) {
            clearInterval(timer);
            terminateSelf();
        }
    }, LISTEN_SOCKET_CHECK_INTERVAL);
    if (typeof timer.unref === 'function') {
        timer.unref();
    }
}


function startApplication() {
    var appRoot = process.env.LSNODE_ROOT || process.cwd();
    var startupFile = process.env.LSNODE_STARTUP_FILE || 'app.js';
    LsNode.listenDone = false;

    if (process.env.LSNODE_ROOT != undefined) {
        try {
            process.chdir(process.env.LSNODE_ROOT);
        } catch (err) {
            console.error("Error setting directory to: " + 
                          process.env.LSNODE_ROOT + ": " + err);
        }
    }
    if (!startupFile.startsWith('/')) {
        if (!appRoot.endsWith('/')) {
            appRoot = appRoot + '/';
        }
        startupFile = appRoot + startupFile;
    }

    process.title = 'lsnode:' + appRoot;

    var consoleLog = process.env.LSNODE_CONSOLE_LOG || '/dev/null';
    fs.closeSync(1);
    try {
        fs.openSync(consoleLog, "a");
    } catch(e) {
        fs.openSync('/dev/null', "a");
    }
    
    http.Server.prototype.realListen = http.Server.prototype.listen;
    http.Server.prototype.listen = customListen;
    http.Server.prototype.address = lsnode_address;
    var app = startupFile.endsWith(".mjs")
        ? import(startupFile)
        : Promise.resolve(require(startupFile));
    app.then((app) => {
        if (!LsNode.listenDone) {
            if (typeof app.listen === "function") {
                app.listen(3000);
            }
        }
    });
}


function lsnode_address() {
    return process.env.LSNODE_SOCKET;
}


function customListen(port) {
    function onListenError(error) {
        restoreBindMask();
        server.emit('error', error);
    }
    function restoreBindMask() {
        // Idempotent, listen() may fail synchronously, asynchronously, or not
        // at all; the mask must go back exactly once in every case, an
        // application that handles the error keeps running with it otherwise.
        if (lsBindMask !== undefined) {
            var mask = lsBindMask;
            lsBindMask = undefined;
            process.umask(mask);
        }
    }
    // The replacement for the listen call!
    var server = this;
    if (LsNode.listenDone) {
        console.error("http.Server.listen() was called more than once, ignore.");
        return server;
    }
    LsNode.listenDone = true;

    var listeners = server.listeners('request');
    var i;
    server.removeAllListeners('request');
    server.on('request', function(req) {
        req.connection.__defineGetter__('remoteAddress', function() {
            return '127.0.0.1';
        });
        req.connection.__defineGetter__('remotePort', function() {
            return port;
        });
        req.connection.__defineGetter__('addressType', function() {
            return 4;
        });
    });
    for (i = 0; i < listeners.length; i++) {
        server.on('request', listeners[i]);
    }

    var callback;
    if (arguments.length > 1 && typeof(arguments[arguments.length - 1]) == 'function') {
        callback = arguments[arguments.length - 1];
    }
    server.once('error', onListenError);
    var lsBindMask;
    var lsBindPath = process.env.LSNODE_BIND_SOCKET
        ? process.env.LSNODE_SOCKET : null;
    if (lsBindPath) {
        // The application owns and binds the unix socket; LiteSpeed connects to
        // it by path. Remove any stale socket from a previous run, and bind
        // under a umask that makes it group-writable (0660) so the web server
        // worker -- which shares the socket's group via the set-gid directory --
        // can connect.
        try { fs.unlinkSync(lsBindPath); } catch (e) {}
        lsBindMask = process.umask(0o117);
    }
    try {
        server.realListen(socketObject, function() {
            if (lsBindPath) {
                // bind() applied the mask above, this covers the socket being
                // created after it was restored, and is a no-op otherwise.
                try { fs.chmodSync(lsBindPath, 0o660); } catch (e) {}
                restoreBindMask();
            }
            // Inherited-fd and app-bind listeners are both reached through
            // LSNODE_SOCKET. Stop if its pathname no longer refers to this
            // listener, regardless of how the listener was obtained.
            if (process.env.LSNODE_SOCKET) {
                watchListeningSocket(process.env.LSNODE_SOCKET,
                                     lsBindPath !== null);
            }
            server.removeListener('error', onListenError);
            if (callback) {
                callback.call(server);
            }
        });
    } catch (err) {
        restoreBindMask();
        throw err;
    }
    // Binding a unix socket finishes inside listen(), so the mask has done its
    // job by the time it returns.  Restore it here rather than from the
    // callback above: node emits 'listening' to the application's own handlers
    // first, and those must not run under our mask.
    restoreBindMask();
    return server;
}
