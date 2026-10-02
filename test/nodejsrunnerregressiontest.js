/*
 * Copyright 2026 Lite Speed Technologies Inc, All Rights Reserved.
 * LITE SPEED PROPRIETARY/CONFIDENTIAL.
 */

'use strict';

var assert = require('assert');
var cp = require('child_process');
var EventEmitter = require('events').EventEmitter;
var fs = require('fs');
var os = require('os');
var path = require('path');
var vm = require('vm');
// Keep VM assertions on Node while optionally running the real children on Bun.
var runtime = process.env.OLS_RUNNER_RUNTIME || process.execPath;

var runners = process.argv.slice(2).map(function(file) {
    return path.resolve(file);
});
assert.strictEqual(runners.length, 2, 'supply both runner scripts');
var testRoot = fs.mkdtempSync(path.join(os.tmpdir(), 'ols-node-regression-'));

// Exercise namespace observations without requiring privileged namespaces.
// The real process tests below cover native signal delivery and fork IPC.
function loadRunner(runner) {
    var proc = new EventEmitter();
    proc.env = { LSNODE_KEEP_CHILDREN: '1' };
    proc.ppid = 77;
    proc.connected = true;
    var ticks = [];
    var nextTicks = [];
    var exits = [];
    proc.nextTick = function(fn) { nextTicks.push(fn); };
    proc.exit = function(code) { exits.push(code); };
    var context = vm.createContext({
        require: require,
        module: {},
        process: proc,
        setInterval: function(fn) {
            ticks.push(fn);
            return { unref: function() {} };
        },
        clearInterval: function() {}
    });
    vm.runInContext(fs.readFileSync(runner, 'utf8'), context,
                    { filename: runner });
    proc.env = {};
    var stopped = 0;
    context.terminateSelf = function() { stopped++; };
    return {
        context: context, proc: proc, ticks: ticks, exits: exits,
        flush: function() {
            while (nextTicks.length) nextTicks.shift()();
        },
        stopped: function() { return stopped; }
    };
}

function checkParentObservations(runner) {
    [0, 1, 88].forEach(function(observed) {
        var test = loadRunner(runner);
        test.proc.ppid = observed;
        test.context.watchParent(77);
        test.ticks[0]();
        assert.strictEqual(test.stopped(), 0, 'live IPC with PPID ' + observed);
        test.proc.connected = false;
        test.proc.emit('disconnect');
        assert.strictEqual(test.stopped(), 1, 'closed IPC with ambiguous PPID');
    });
    var test = loadRunner(runner);
    test.context.watchParent(77);
    test.proc.connected = false;
    test.proc.emit('disconnect');
    assert.strictEqual(test.stopped(), 0, 'intentional disconnect with same PPID');
    test.proc.ppid = 88;
    test.ticks[0]();
    assert.strictEqual(test.stopped(), 1, 'reparenting to a subreaper');

    test = loadRunner(runner);
    test.proc.ppid = 88;
    test.proc.connected = false;
    test.context.watchParent(77);
    assert.strictEqual(test.stopped(), 1, 'parent died before preload ran');
}

function checkSignalOwnership(runner) {
    ['once', 'prependOnceListener'].forEach(function(method) {
        [true, false].forEach(function(before) {
            var test = loadRunner(runner);
            var child = new EventEmitter();
            var called = 0;
            function appHandler() { called++; }
            test.context.observeSignalDispatch();
            if (before) test.proc[method]('SIGTERM', appHandler);
            test.context.trackChild(child, false);
            if (!before) test.proc[method]('SIGTERM', appHandler);
            test.flush();
            test.proc.emit('SIGTERM');
            assert.strictEqual(called, 1);
            assert.strictEqual(test.exits.length, 0, 'honor application once handler');
            test.flush();
            test.proc.emit('SIGTERM');
            assert.deepStrictEqual(test.exits, [143], 'next signal uses default exit');
            child.emit('exit');
            child.emit('close');
            assert.strictEqual(test.proc.listenerCount('SIGTERM'), 0);
        });
    });
}

function checkSocketErrors(runner) {
    ['ENOENT', 'EACCES'].forEach(function(code) {
        [false, true].forEach(function(boundSelf) {
            var test = loadRunner(runner);
            test.context.fs = { lstatSync: function() {
                throw Object.assign(new Error(code), { code: code });
            } };
            test.context.watchListeningSocket('/test/socket', boundSelf);
            assert.strictEqual(test.stopped(), boundSelf ? 1 : 0);
        });
    });
    var test = loadRunner(runner);
    var error;
    test.context.fs = { lstatSync: function() {
        if (error) throw Object.assign(new Error(error), { code: error });
        return { isSocket: function() { return true; }, dev: 1, ino: 2 };
    } };
    test.context.watchListeningSocket('/test/socket', false);
    error = 'EACCES';
    test.ticks[0]();
    assert.strictEqual(test.stopped(), 0, 'stat permission error is not removal');
    error = 'ENOENT';
    test.ticks[0]();
    assert.strictEqual(test.stopped(), 1, 'removal after successful baseline');
}

function delay(ms) {
    return new Promise(function(resolve) { setTimeout(resolve, ms); });
}

async function waitUntil(predicate, label, timeout) {
    var deadline = Date.now() + (timeout || 1500);
    while (!predicate()) {
        if (Date.now() >= deadline) throw new Error('timed out: ' + label);
        await delay(10);
    }
}

function exited(child) {
    return child.exitCode !== null || child.signalCode !== null;
}

async function stop(child) {
    if (!exited(child)) child.kill('SIGKILL');
    await waitUntil(function() { return exited(child); }, 'cleanup');
}

async function checkBusySignal(runner, app, mode) {
    var ready = path.join(testRoot, 'ready');
    if (fs.existsSync(ready)) fs.unlinkSync(ready);
    var env = Object.assign({}, process.env, {
        LSNODE_ROOT: testRoot, LSNODE_STARTUP_FILE: app,
        LSNODE_CONSOLE_LOG: path.join(testRoot, 'console.log'),
        TEST_READY: ready, TEST_MODE: mode,
        TEST_WORKER: path.join(testRoot, 'worker.js')
    });
    delete env.LSNODE_KEEP_CHILDREN;
    delete env.LSNODE_GUARD_PPID;
    var child = cp.spawn(runtime, [runner], {
        env: env, stdio: ['ignore', 'ignore', 'inherit']
    });
    try {
        await waitUntil(function() { return fs.existsSync(ready); }, mode + ' ready', 5000);
        var start = Date.now();
        child.kill('SIGTERM');
        if (mode.indexOf('once') >= 0) {
            await waitUntil(function() {
                return fs.readFileSync(ready, 'utf8') === 'handled';
            }, mode + ' application signal handler');
            assert.strictEqual(exited(child), false, 'application owns first signal');
            child.kill('SIGTERM');
        }
        await waitUntil(function() { return exited(child); }, mode + ' native SIGTERM');
        console.log(path.basename(runner) + ' ' + mode + ': SIGTERM in ' +
                    (Date.now() - start) + ' ms');
    } finally {
        await stop(child);
    }
}

async function checkMismatchIPC(runner) {
    var env = Object.assign({}, process.env, { LSNODE_GUARD_PPID: '99999999' });
    delete env.LSNODE_KEEP_CHILDREN;
    var child = cp.fork(path.join(testRoot, 'worker.js'), [], {
        env: env, execPath: runtime, execArgv: ['--require', runner], silent: true
    });
    try {
        await delay(1250);
        assert.strictEqual(exited(child), false, 'mismatched PPID with real live IPC');
        child.disconnect();
        await waitUntil(function() { return exited(child); }, 'ambiguous IPC shutdown');
    } finally {
        await stop(child);
    }
}

var appSource = [
    "var fs = require('fs'), cp = require('child_process');",
    "function busy() {",
    "    fs.writeFileSync(process.env.TEST_READY, 'ready');",
    "    var end = Date.now() + 6000;",
    "    while (Date.now() < end) {}",
    "    setInterval(function() {}, 1000);",
    "}",
    "var mode = process.env.TEST_MODE;",
    "if (mode.indexOf('once') >= 0) {",
    "    function handled() { fs.writeFileSync(process.env.TEST_READY, 'handled'); }",
    "    if (mode === 'once_before') process.once('SIGTERM', handled);",
    "    cp.fork(process.env.TEST_WORKER);",
    "    if (mode === 'once_after') process.once('SIGTERM', handled);",
    "    if (mode === 'prepend_once') process.prependOnceListener('SIGTERM', handled);",
    "    setTimeout(function() { fs.writeFileSync(process.env.TEST_READY, 'ready'); }, 100);",
    "    setInterval(function() {}, 1000);",
    "}",
    "else if (mode === 'none') busy();",
    "else {",
    "    var failed = process.env.TEST_MODE === 'failed';",
    "    var child = cp.spawn(failed ? '/nonexistent/ols-test' : process.execPath,",
    "                         failed ? [] : ['-e', '']);",
    "    child.on('error', function() {});",
    "    child.on('close', function() { setImmediate(busy); });",
    "}"
].join('\n');

(async function() {
    try {
        fs.writeFileSync(path.join(testRoot, 'app.js'), appSource);
        fs.writeFileSync(path.join(testRoot, 'app.mjs'),
            "import { createRequire } from 'module';\n" +
            "const require = createRequire(import.meta.url);\n" + appSource);
        fs.writeFileSync(path.join(testRoot, 'worker.js'),
                         'setInterval(function() {}, 1000);\n');
        for (var i = 0; i < runners.length; i++) {
            checkParentObservations(runners[i]);
            checkSignalOwnership(runners[i]);
            checkSocketErrors(runners[i]);
            for (var mode of ['none', 'exited', 'failed', 'once_before',
                             'once_after', 'prepend_once']) {
                await checkBusySignal(runners[i], i ? 'app.mjs' : 'app.js', mode);
            }
            await checkMismatchIPC(runners[i]);
        }
        console.log('Node runner regression tests passed');
    } finally {
        fs.rmSync(testRoot, { recursive: true, force: true });
    }
})().catch(function(err) {
    console.error(err.stack || err);
    process.exitCode = 1;
});
