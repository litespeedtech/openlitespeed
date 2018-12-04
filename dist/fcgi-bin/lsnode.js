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
module.isApplicationLoader = true;
global.LsNode = new EventEmitter();
startApplication();

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
    if (startupFile.slice(0,0) != '/') {
        startupFile = appRoot + '/' + startupFile;
    }

    process.title = 'lsnode:' + appRoot;

    var consoleLog = process.env.LSNODE_CONSOLE_LOG || '/dev/null';
    fs.closeSync(1);
    try {
        fs.openSync(consoleLog, "w+");
    } catch(e) {
        fs.openSync('/dev/null', "w+");
    }
    
    http.Server.prototype.realListen = http.Server.prototype.listen;
    http.Server.prototype.listen = customListen;
    require(startupFile);
}

function customListen(port) {
    function onListenError(error) {
        server.emit('error', error);
    }
    // The replacement for the listen call!
    var server = this;
    if (LsNode.listenDone) {
        throw new Error("http.Server.listen() was called more than once " +
                        "which is not allowed.");
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




