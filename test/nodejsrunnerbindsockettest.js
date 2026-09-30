/*
 * Copyright 2026 Lite Speed Technologies Inc, All Rights Reserved.
 * LITE SPEED PROPRIETARY/CONFIDENTIAL.
 */

'use strict';

var childProcess = require('child_process');
var fs = require('fs');
var net = require('net');
var os = require('os');
var path = require('path');

var runners = process.argv.slice(2);
if (runners.length !== 2) {
    console.error('usage: node nodejsrunnerbindsockettest.js lsnode.js ' +
                  'lsnodesm.js');
    process.exit(2);
}

// Distinctive, and different from the 0117 the runner binds under.
var TEST_UMASK = 0o027;
process.umask(TEST_UMASK);

var testRoot = fs.mkdtempSync(path.join(os.tmpdir(), 'ols-node-bind-'));
var appPath = path.join(testRoot, 'app.js');
var livenessAppPath = path.join(testRoot, 'liveness-app.js');
var sequence = 0;

// The application reports what listen() did and the umask it is left with, so
// a mask the runner failed to restore shows up as the application's own mask.
fs.writeFileSync(appPath, [
    "var fs = require('fs');",
    "var http = require('http');",
    "function report(what) {",
    "    var mask = process.umask(0o077);",
    "    fs.writeFileSync(process.env.TEST_REPORT_FILE,",
    "                     what + ' ' + mask.toString(8));",
    "    process.exit(0);",
    "}",
    "var server = http.createServer(function(req, res) { res.end('ok'); });",
    "server.on('listening', function() { report('listening'); });",
    "server.on('error', function(err) { report('error:' + err.code); });",
    "try {",
    "    server.listen(3000);",
    "} catch (err) {",
    "    report('throw:' + err.code);",
    "}",
    ""
].join('\n'));

// Keep the runner alive after it has bound its socket.  The parent test removes
// or replaces that path and expects this unreachable instance to stop itself.
fs.writeFileSync(livenessAppPath, [
    "var fs = require('fs');",
    "var http = require('http');",
    "var server = http.createServer(function(req, res) { res.end('ok'); });",
    "var events = 0, callbacks = 0;",
    "server.on('listening', function() { events++; });",
    "server.listen(3000, function() {",
    "    if (this !== server) throw new Error('wrong listen callback receiver');",
    "    callbacks++;",
    "    setTimeout(function() {",
    "        if (events !== 1 || callbacks !== 1) throw new Error('duplicate listening notification');",
    "        fs.writeFileSync(process.env.TEST_LIVENESS_FILE, String(process.pid));",
    "    }, 50);",
    "});",
    "setInterval(function() {}, 1000);",
    ""
].join('\n'));


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


async function runScenario(runnerPath, socketPath, expectSuccess) {
    sequence++;
    var reportFile = path.join(testRoot, 'report-' + sequence + '.txt');
    var label = path.basename(runnerPath) + ' ' +
                (expectSuccess ? 'bind' : 'failed bind');

    var env = Object.assign({}, process.env, {
        LSNODE_BIND_SOCKET: '1',
        LSNODE_SOCKET: socketPath,
        LSNODE_CONSOLE_LOG: path.join(testRoot, 'console-' + sequence + '.log'),
        LSNODE_ROOT: testRoot,
        LSNODE_STARTUP_FILE: 'app.js',
        TEST_REPORT_FILE: reportFile
    });
    delete env.LSNODE_KEEP_CHILDREN;

    var runner = childProcess.spawn(process.execPath, [runnerPath], {
        env: env,
        stdio: ['ignore', 'ignore', 'inherit']
    });
    try {
        await waitUntil(function() {
            return fs.existsSync(reportFile);
        }, 5000, label + ' to report');
        var report = fs.readFileSync(reportFile, 'ascii').split(' ');
        var outcome = report[0];
        var mask = parseInt(report[1], 8);

        if (expectSuccess) {
            if (outcome !== 'listening') {
                throw new Error(label + ': expected a bound socket, got ' +
                                outcome);
            }
            var mode = fs.statSync(socketPath).mode & 0o777;
            if (mode !== 0o660) {
                throw new Error(label + ': socket mode is 0' +
                                mode.toString(8) + ', expected 0660');
            }
        } else if (outcome.indexOf('error:') !== 0 &&
                   outcome.indexOf('throw:') !== 0) {
            throw new Error(label + ': expected listen() to fail, got ' +
                            outcome);
        }

        if (mask !== TEST_UMASK) {
            throw new Error(label + ': application left with umask 0' +
                            mask.toString(8) + ', expected 0' +
                            TEST_UMASK.toString(8));
        }
    } finally {
        try {
            runner.kill('SIGKILL');
        } catch (err) {
        }
    }
}


function listenReplacement(server, socketPath) {
    return new Promise(function(resolve, reject) {
        server.once('error', reject);
        server.listen(socketPath, function() {
            server.removeListener('error', reject);
            resolve();
        });
    });
}


function closeServer(server) {
    if (!server.listening) {
        return Promise.resolve();
    }
    return new Promise(function(resolve) {
        server.close(function() {
            resolve();
        });
    });
}


async function runAppBindSocketLivenessScenario(runnerPath, replace, noWatch) {
    sequence++;
    var socketPath = path.join(testRoot, 'liveness-' + sequence + '.sock');
    var reportFile = path.join(testRoot, 'liveness-' + sequence + '.pid');
    var label = path.basename(runnerPath) + ' socket ' +
                (replace ? 'replacement' : 'removal');
    var env = Object.assign({}, process.env, {
        LSNODE_BIND_SOCKET: '1',
        LSNODE_SOCKET: socketPath,
        LSNODE_CONSOLE_LOG: path.join(testRoot, 'console-' + sequence + '.log'),
        LSNODE_ROOT: testRoot,
        LSNODE_STARTUP_FILE: path.basename(livenessAppPath),
        TEST_LIVENESS_FILE: reportFile
    });
    delete env.LSNODE_KEEP_CHILDREN;
    env.LSNODE_NO_SOCKET_WATCH = noWatch ? '1' : '0';

    var runner = childProcess.spawn(process.execPath, [runnerPath], {
        env: env,
        stdio: ['ignore', 'ignore', 'inherit']
    });
    var replacement;
    try {
        await waitUntil(function() {
            return fs.existsSync(reportFile) && fs.existsSync(socketPath);
        }, 5000, label + ' setup');

        fs.unlinkSync(socketPath);
        if (replace) {
            replacement = net.createServer();
            await listenReplacement(replacement, socketPath);
        }
        if (noWatch) {
            await assertStillRunning(runner);
        } else {
            await waitForExit(runner, 5000);
        }
    } finally {
        if (runner.exitCode === null && runner.signalCode === null) {
            runner.kill('SIGKILL');
        }
        if (replacement) {
            await closeServer(replacement);
        }
    }
}


async function runInheritedSocketLivenessScenario(runnerPath, replace, noWatch,
                                                   invisiblePath) {
    sequence++;
    var socketPath = path.join(testRoot, 'inherited-' + sequence + '.sock');
    var reportFile = path.join(testRoot, 'inherited-' + sequence + '.pid');
    var label = path.basename(runnerPath) + ' inherited socket ' +
                (replace ? 'replacement' : 'removal');
    var inherited = net.createServer();
    var replacement;
    var runner;
    await listenReplacement(inherited, socketPath);
    if (!inherited._handle || typeof inherited._handle.fd !== 'number') {
        await closeServer(inherited);
        throw new Error(label + ': test runtime cannot expose a listening fd');
    }
    var env = Object.assign({}, process.env, {
        LSNODE_SOCKET: invisiblePath ? socketPath + '.outside' : socketPath,
        LSNODE_CONSOLE_LOG: path.join(testRoot, 'console-' + sequence + '.log'),
        LSNODE_ROOT: testRoot,
        LSNODE_STARTUP_FILE: path.basename(livenessAppPath),
        TEST_LIVENESS_FILE: reportFile
    });
    delete env.LSNODE_BIND_SOCKET;
    delete env.LSNODE_KEEP_CHILDREN;
    env.LSNODE_NO_SOCKET_WATCH = noWatch ? '1' : '0';

    runner = childProcess.spawn(process.execPath, [runnerPath], {
        env: env,
        stdio: [inherited._handle.fd, 'ignore', 'inherit']
    });
    try {
        await waitUntil(function() {
            return fs.existsSync(reportFile);
        }, 5000, label + ' setup');

        if (replace) {
            await closeServer(inherited);
            replacement = net.createServer();
            await listenReplacement(replacement, socketPath);
        } else {
            fs.unlinkSync(socketPath);
        }
        if (noWatch || invisiblePath) {
            await assertStillRunning(runner);
        } else {
            await waitForExit(runner, 5000);
        }
    } finally {
        if (runner && runner.exitCode === null && runner.signalCode === null) {
            runner.kill('SIGKILL');
        }
        if (replacement) {
            await closeServer(replacement);
        }
        await closeServer(inherited);
    }
}


async function assertStillRunning(runner) {
    await new Promise(function(resolve) { setTimeout(resolve, 1250); });
    if (runner.exitCode !== null || runner.signalCode !== null) {
        throw new Error('runner exited with disabled/unavailable socket watch');
    }
}


function cleanup() {
    try {
        fs.rmSync(testRoot, { recursive: true, force: true });
    } catch (err) {
    }
}


(async function() {
    try {
        for (var i = 0; i < runners.length; i++) {
            await runScenario(runners[i],
                              path.join(testRoot, 'ok-' + i + '.sock'), true);
            // A directory that does not exist, so bind() cannot succeed.
            await runScenario(runners[i],
                              path.join(testRoot, 'missing', 'bad.sock'),
                              false);
            await runAppBindSocketLivenessScenario(runners[i], false);
            await runAppBindSocketLivenessScenario(runners[i], true);
            await runAppBindSocketLivenessScenario(runners[i], true, true);
            // Bun uses app-bind mode because it cannot adopt fd 0 as a listener.
            if (!process.versions.bun) {
                await runInheritedSocketLivenessScenario(runners[i], false);
                await runInheritedSocketLivenessScenario(runners[i], true);
                await runInheritedSocketLivenessScenario(runners[i], true, true);
                await runInheritedSocketLivenessScenario(runners[i], false, false,
                                                         true);
            }
        }
        console.log('Node runner bind socket tests passed');
    } finally {
        cleanup();
    }
})().catch(function(err) {
    console.error(err.stack || err);
    process.exitCode = 1;
});
