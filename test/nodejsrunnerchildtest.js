/*
 * Copyright 2026 Lite Speed Technologies Inc, All Rights Reserved.
 * LITE SPEED PROPRIETARY/CONFIDENTIAL.
 */

'use strict';

var childProcess = require('child_process');
var fs = require('fs');
var os = require('os');
var path = require('path');

var runners = process.argv.slice(2);
if (runners.length !== 2) {
    console.error('usage: node nodejsrunnerchildtest.js lsnode.js lsnodesm.js');
    process.exit(2);
}

var testRoot = fs.mkdtempSync(path.join(os.tmpdir(), 'ols-node-runner-'));
var workerPath = path.join(testRoot, 'worker.js');
var grandchildPath = path.join(testRoot, 'grandchild.js');
var appPath = path.join(testRoot, 'app.js');
var esmAppPath = path.join(testRoot, 'app.mjs');
var createdFiles = [workerPath, grandchildPath, appPath, esmAppPath];
var sequence = 0;

fs.writeFileSync(workerPath, [
    "var cp = require('child_process');",
    "var fs = require('fs');",
    "fs.writeFileSync(process.env.TEST_CHILD_PID_FILE, String(process.pid));",
    "if (process.env.TEST_GRANDCHILD_PID_FILE) {",
    "    cp.fork(process.env.TEST_GRANDCHILD);",
    "}",
    "if (process.env.TEST_DISCONNECT === '1' && process.connected) {",
    "    process.disconnect();",
    "}",
    "setInterval(function() {}, 1000);",
    ""
].join('\n'));

fs.writeFileSync(grandchildPath, [
    "var fs = require('fs');",
    "fs.writeFileSync(process.env.TEST_GRANDCHILD_PID_FILE, String(process.pid));",
    "process.on('SIGTERM', function() {});",
    "setInterval(function() {}, 1000);",
    ""
].join('\n'));

var appSource = [
    "var cp = require('child_process');",
    "if (process.env.TEST_PRIVATE_ENV === '1') {",
    "    cp.fork(process.env.TEST_WORKER, null, {",
    "        env: Object.assign({}, process.env)",
    "    });",
    "} else {",
    "    cp.fork(process.env.TEST_WORKER);",
    "}",
    "setInterval(function() {}, 1000);",
    ""
].join('\n');
fs.writeFileSync(appPath, appSource);

fs.writeFileSync(esmAppPath, [
    "import { createRequire } from 'module';",
    "const require = createRequire(import.meta.url);",
    appSource,
    ""
].join('\n'));


function delay(milliseconds) {
    return new Promise(function(resolve) {
        setTimeout(resolve, milliseconds);
    });
}


function processIsAlive(pid) {
    try {
        process.kill(pid, 0);
    } catch (err) {
        return false;
    }

    // kill(pid, 0) also succeeds for a zombie.  Treat it as gone so this test
    // does not depend on how quickly PID 1 reaps adopted children.
    try {
        var stat = fs.readFileSync('/proc/' + pid + '/stat', 'ascii');
        var state = stat.substring(stat.lastIndexOf(')') + 2).split(' ')[0];
        return state !== 'Z';
    } catch (err) {
        return true;
    }
}


function waitUntil(predicate, timeout, description) {
    var deadline = Date.now() + timeout;
    return new Promise(function(resolve, reject) {
        function check() {
            if (predicate()) {
                resolve();
            } else if (Date.now() >= deadline) {
                reject(new Error('timed out waiting for ' + description));
            } else {
                setTimeout(check, 25);
            }
        }
        check();
    });
}


function waitForExit(child, timeout) {
    if (child.exitCode !== null || child.signalCode !== null) {
        return Promise.resolve();
    }
    return new Promise(function(resolve, reject) {
        var timer = setTimeout(function() {
            reject(new Error('runner ' + child.pid + ' did not exit'));
        }, timeout);
        child.once('exit', function() {
            clearTimeout(timer);
            resolve();
        });
    });
}


async function runScenario(runnerPath, startupFile, signal, disconnect,
                           privateEnv, withGrandchild) {
    sequence++;
    var pidFile = path.join(testRoot, 'child-' + sequence + '.pid');
    var logFile = path.join(testRoot, 'console-' + sequence + '.log');
    createdFiles.push(pidFile, logFile);
    var grandchildPidFile = '';
    if (withGrandchild) {
        grandchildPidFile = path.join(testRoot,
                                     'grandchild-' + sequence + '.pid');
        createdFiles.push(grandchildPidFile);
    }

    var env = Object.assign({}, process.env, {
        LSNODE_CONSOLE_LOG: logFile,
        LSNODE_ROOT: testRoot,
        LSNODE_STARTUP_FILE: startupFile,
        TEST_CHILD_PID_FILE: pidFile,
        TEST_DISCONNECT: disconnect ? '1' : '0',
        TEST_GRANDCHILD: grandchildPath,
        TEST_GRANDCHILD_PID_FILE: grandchildPidFile,
        TEST_PRIVATE_ENV: privateEnv ? '1' : '0',
        TEST_WORKER: workerPath
    });
    delete env.LSNODE_KEEP_CHILDREN;

    var runner = childProcess.spawn(process.execPath, [runnerPath], {
        env: env,
        stdio: ['ignore', 'ignore', 'inherit']
    });
    var childPid = 0;
    var grandchildPid = 0;
    try {
        await waitUntil(function() {
            return fs.existsSync(pidFile);
        }, 5000, path.basename(runnerPath) + ' child startup');
        childPid = parseInt(fs.readFileSync(pidFile, 'ascii'), 10);
        if (!processIsAlive(childPid)) {
            throw new Error('forked child ' + childPid + ' exited early');
        }
        if (withGrandchild) {
            await waitUntil(function() {
                return fs.existsSync(grandchildPidFile);
            }, 5000, 'grandchild startup');
            grandchildPid = parseInt(fs.readFileSync(grandchildPidFile,
                                                     'ascii'), 10);
            if (!processIsAlive(grandchildPid)) {
                throw new Error('grandchild ' + grandchildPid +
                                ' exited early');
            }
        }

        if (disconnect) {
            await delay(1250);
            if (!processIsAlive(childPid)) {
                throw new Error('IPC disconnect killed child while parent lived');
            }
        }

        runner.kill(signal);
        await waitForExit(runner, 5000);
        await waitUntil(function() {
            return !processIsAlive(childPid);
        }, 5000, 'forked child ' + childPid + ' shutdown after ' + signal);
        if (grandchildPid) {
            await waitUntil(function() {
                return !processIsAlive(grandchildPid);
            }, 5000, 'grandchild ' + grandchildPid + ' shutdown');
        }
    } finally {
        if (processIsAlive(runner.pid)) {
            runner.kill('SIGKILL');
        }
        if (childPid && processIsAlive(childPid)) {
            process.kill(childPid, 'SIGKILL');
        }
        if (grandchildPid && processIsAlive(grandchildPid)) {
            process.kill(grandchildPid, 'SIGKILL');
        }
    }
}


// fork() may name another runtime through execPath.  A node install reached
// through a symlink of any name must still get the watchdog -- SIGKILL is the
// only signal that proves it, a tracked child dies on SIGTERM either way --
// while something that is not a known runtime must be started untouched.
async function runExecPathScenario(runnerPath) {
    sequence++;
    var binDir = path.join(testRoot, 'bin-' + sequence);
    // Named like a version manager's "current" link, nothing about the name
    // says node; only resolving it does.
    var linkedRuntime = path.join(binDir, 'current');
    var otherRuntime = path.join(binDir, 'mystery');
    fs.mkdirSync(binDir);
    fs.symlinkSync(process.execPath, linkedRuntime);
    fs.writeFileSync(otherRuntime, [
        '#!/bin/sh',
        'echo "$$ $@" > "$TEST_REPORT_FILE"',
        'sleep 30',
        ''
    ].join('\n'), { mode: 0o755 });

    var reportPath = path.join(testRoot, 'execpath-worker-' + sequence + '.js');
    var appExecPath = path.join(testRoot, 'execpath-app-' + sequence + '.js');
    var linkedFile = path.join(testRoot, 'linked-' + sequence + '.json');
    var otherFile = path.join(testRoot, 'other-' + sequence + '.txt');
    createdFiles.push(reportPath, appExecPath, linkedFile, otherFile);

    fs.writeFileSync(reportPath, [
        "var fs = require('fs');",
        "fs.writeFileSync(process.env.TEST_REPORT_FILE, JSON.stringify({",
        "    pid: process.pid, execArgv: process.execArgv",
        "}));",
        "setInterval(function() {}, 1000);",
        ""
    ].join('\n'));
    fs.writeFileSync(appExecPath, [
        "var cp = require('child_process');",
        "cp.fork(process.env.TEST_REPORT_WORKER, [], {",
        "    execPath: process.env.TEST_LINKED_RUNTIME,",
        "    env: Object.assign({}, process.env,",
        "                       { TEST_REPORT_FILE: process.env.TEST_LINKED_FILE })",
        "});",
        "cp.fork(process.env.TEST_REPORT_WORKER, [], {",
        "    execPath: process.env.TEST_OTHER_RUNTIME,",
        "    env: Object.assign({}, process.env,",
        "                       { TEST_REPORT_FILE: process.env.TEST_OTHER_FILE })",
        "});",
        "setInterval(function() {}, 1000);",
        ""
    ].join('\n'));

    var env = Object.assign({}, process.env, {
        LSNODE_CONSOLE_LOG: path.join(testRoot, 'console-' + sequence + '.log'),
        LSNODE_ROOT: testRoot,
        LSNODE_STARTUP_FILE: path.basename(appExecPath),
        TEST_REPORT_WORKER: reportPath,
        TEST_LINKED_RUNTIME: linkedRuntime,
        TEST_OTHER_RUNTIME: otherRuntime,
        TEST_LINKED_FILE: linkedFile,
        TEST_OTHER_FILE: otherFile
    });
    delete env.LSNODE_KEEP_CHILDREN;
    createdFiles.push(env.LSNODE_CONSOLE_LOG);

    var runner = childProcess.spawn(process.execPath, [runnerPath], {
        env: env,
        stdio: ['ignore', 'ignore', 'inherit']
    });
    var linkedPid = 0;
    var otherPid = 0;
    try {
        await waitUntil(function() {
            return fs.existsSync(linkedFile) && fs.existsSync(otherFile);
        }, 5000, 'children with an explicit execPath to start');

        var linked = JSON.parse(fs.readFileSync(linkedFile, 'ascii'));
        linkedPid = linked.pid;
        if (linked.execArgv.indexOf('--require') < 0 ||
            linked.execArgv.indexOf(path.resolve(runnerPath)) < 0) {
            throw new Error('node reached through a symlink did not get the ' +
                            'watchdog preloaded: ' +
                            JSON.stringify(linked.execArgv));
        }

        var other = fs.readFileSync(otherFile, 'ascii').trim().split(' ');
        otherPid = parseInt(other[0], 10);
        if (other.indexOf('--require') >= 0) {
            throw new Error('unknown runtime was given flags it may reject: ' +
                            other.slice(1).join(' '));
        }

        // SIGKILL: the runner cannot clean up, so only a child that preloaded
        // the watchdog notices its parent is gone.
        runner.kill('SIGKILL');
        await waitForExit(runner, 5000);
        await waitUntil(function() {
            return !processIsAlive(linkedPid);
        }, 5000, 'symlinked node child ' + linkedPid + ' to notice the ' +
                 'parent SIGKILL');
    } finally {
        if (processIsAlive(runner.pid)) {
            runner.kill('SIGKILL');
        }
        // The unknown runtime is left running on purpose, it has no watchdog.
        if (linkedPid && processIsAlive(linkedPid)) {
            process.kill(linkedPid, 'SIGKILL');
        }
        if (otherPid && processIsAlive(otherPid)) {
            process.kill(otherPid, 'SIGKILL');
        }
        try {
            fs.unlinkSync(linkedRuntime);
            fs.unlinkSync(otherRuntime);
            fs.rmdirSync(binDir);
        } catch (err) {
        }
    }
}


function cleanup() {
    for (var i = 0; i < createdFiles.length; i++) {
        try {
            fs.unlinkSync(createdFiles[i]);
        } catch (err) {
        }
    }
    try {
        fs.rmdirSync(testRoot);
    } catch (err) {
    }
}


(async function() {
    try {
        await runScenario(runners[0], appPath, 'SIGTERM', false, false, false);
        await runScenario(runners[0], appPath, 'SIGKILL', false, false, false);
        await runScenario(runners[0], appPath, 'SIGKILL', true, true, false);
        await runScenario(runners[1], esmAppPath, 'SIGTERM', false, true,
                          false);
        await runScenario(runners[1], esmAppPath, 'SIGKILL', false, false,
                          true);
        await runExecPathScenario(runners[0]);
        console.log('Node runner child supervision tests passed');
    } finally {
        cleanup();
    }
})().catch(function(err) {
    console.error(err.stack || err);
    process.exitCode = 1;
});
