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

var socketObject = { fd: 0 };

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
var FORCE_KILL_DELAY = 3000;
var children = [];
var terminating = false;

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
    child.on('exit', function() {
        var idx = children.indexOf(entry);
        if (idx >= 0) {
            children.splice(idx, 1);
        }
    });
    return child;
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

    var signals = { SIGHUP: 1, SIGINT: 2, SIGQUIT: 3, SIGTERM: 15 };
    Object.keys(signals).forEach(function(name) {
        process.on(name, function() {
            if (process.listenerCount(name) > 1) {
                // The application installed its own handler, leave the
                // shutdown to it, the 'exit' handler cleans up afterwards.
                return;
            }
            // Restore the default behavior of dying on the signal, the 'exit'
            // handler takes the children along.
            process.exit(128 + signals[name]);
        });
    });
}


function watchParent(ppid) {
    if (isKeepChildren() || !ppid) {
        return;
    }
    var timer = setInterval(function() {
        var currentPpid = getParentPid();
        if (currentPpid && currentPpid !== ppid) {
            terminateSelf();
        }
    }, PARENT_CHECK_INTERVAL);
    if (typeof timer.unref === 'function') {
        timer.unref();
    }
    // A dead parent closes the fork() IPC channel.  Applications may also call
    // disconnect() deliberately, so only terminate if the OS parent changed.
    // If re-parenting has not completed yet, the interval catches it shortly.
    process.on('disconnect', function() {
        var currentPpid = getParentPid();
        if (currentPpid && currentPpid !== ppid) {
            terminateSelf();
        }
    });
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
    var app = require(startupFile);
    if (!LsNode.listenDone) {
        if (typeof app.listen === "function")
            app.listen(3000);
    }
}


function lsnode_address() {
    return process.env.LSNODE_SOCKET;
}


function customListen(port) {
    function onListenError(error) {
        server.emit('error', error);
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
    server.realListen(socketObject, function() {
        server.removeListener('error', onListenError);
        if (callback) {
            server.once('listening', callback);
        }
        server.emit('listening');
    });
    return server;
}
