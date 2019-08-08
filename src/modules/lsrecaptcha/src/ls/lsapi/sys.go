//****************************************************************************
//   Open LiteSpeed is an open source HTTP server.                           *
//   Copyright (C) 2013 - 2018  LiteSpeed Technologies, Inc.                 *
//                                                                           *
//   This program is free software: you can redistribute it and/or modify    *
//   it under the terms of the GNU General Public License as published by    *
//   the Free Software Foundation, either version 3 of the License, or       *
//   (at your option) any later version.                                     *
//                                                                           *
//   This program is distributed in the hope that it will be useful,         *
//   but WITHOUT ANY WARRANTY; without even the implied warranty of          *
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the            *
//   GNU General Public License for more details.                            *
//                                                                           *
//   You should have received a copy of the GNU General Public License       *
//   along with this program. If not, see http://www.gnu.org/licenses/.      *
//****************************************************************************

package lsapi

import (
	"fmt"
	"log"
	"os"
	"os/exec"
	"os/signal"
	"path"
	"path/filepath"
	"syscall"
	"time"
)

const (
	runStateStopped int = iota
	runStateRunning
	runStateRestart
)

var sysDir string = "/tmp"

// var pidfile string
var sock string

var is_running int
var chStop chan os.Signal

// func GetPidFile() string { return pidfile }
func GetSock() string { return sock }

func setRunningState(s int) {
	is_running = s
}

func IsRunning() bool {
	return is_running == runStateRunning
}

func Stop() {
	log.Println("Stop signal received.")
	setRunningState(runStateStopped)
}

func GracefulRestart() error {
	log.Println("Try to graceful restart.")
	path, err := exec.LookPath(os.Args[0])
	if err != nil {
		return err
	}

	fd, err := l.File() // TODO: Do I need to close this fd? What about l?
	if err != nil {
		return err
	}

	cmd := exec.Command(path)
	cmd.Env = append(os.Environ(),
		fmt.Sprintf("%s=%s", lsapiRecoverNetwork, l.Addr().Network()),
		fmt.Sprintf("%s=%s", lsapiRecoverAddr, l.Addr().String()),
	)
	cmd.Stdin = os.Stdin
	cmd.Stdout = os.Stdout
	cmd.Stderr = os.Stderr
	cmd.ExtraFiles = []*os.File{
		fd,
	}
	err = cmd.Start()
	if err != nil {
		log.Print(err.Error())
	}
	log.Println("Child process successfully started.")
	return err
}

func initSysVars(newSysDir, newSysFileName string) {

	if newSysDir != "" {
		absDir, err := filepath.Abs(newSysDir)
		if err != nil {
			log.Fatalf("Failed to init lsapi, cannot find absolute path for %s", newSysDir)
		}
		sysDir = absDir
	}

	if err := os.MkdirAll(sysDir, 0750); err != nil {
		log.Fatalf("Failed to init lsapi, failed to create dir %s, err: %+v", sysDir, err)
	}

	if newSysFileName == "" {
		newSysFileName = path.Base(os.Args[0])
		log.Printf("No sys file passed in, use %s", newSysFileName)
	}

	fullSysPath := filepath.Join(sysDir, newSysFileName)

	// pidfile = fullSysPath + ".pid"
	sock = "uds:/" + fullSysPath + ".sock"
}

// initSys initializes the system files and registers signal handlers.
// Parameters newTmp and sysFileName may be empty; if not set, /tmp and
// the executable name will be used respectively for the system files.
// The directory will be created if it does not exist.
func initSys(newSysDir, newSysFileName string) {

	initSysVars(newSysDir, newSysFileName)

	// checkPidFile()

	registerSignalHandler()
}

// registerSignalHandler creates a channel and goroutine to capture a
// termination signal sent to the process.
func registerSignalHandler() {
	log.Println("Register signal handler.")
	chStop = make(chan os.Signal)
	signal.Notify(chStop, syscall.SIGKILL)
	signal.Notify(chStop, syscall.SIGTERM)
	signal.Notify(chStop, syscall.SIGINT)
	signal.Notify(chStop, syscall.SIGUSR1)
	signal.Notify(chStop, syscall.SIGUSR2)

	go handleSignal(chStop)
}

// handleSignal handles the termination signals sent to the process.
// SIGTERM, SIGINT will stop the process.
// SIGUSR1 will attempt to restart it.
func handleSignal(chStop chan os.Signal) {
	// setPidFile()
	// defer os.Remove(GetPidFile())

	for {
		sig := <-chStop
		log.Printf("Caught signal %+v.\n", sig)

		// Run any clean up code here.

		if sig == syscall.SIGUSR1 {
			log.Println("Try to gracefully restart lsapi.")
			// os.Remove(GetPidFile())
			GracefulRestart()
			break
		} else if sig == syscall.SIGUSR2 {
			toggleDebug()
		} else {
			log.Println("Stopping the process.")
			// defer os.Remove(GetPidFile())
			Stop()
			break
		}
	}
	// Give main routine a chance to stop.
	time.Sleep(2 * time.Second)
	closeLogFile()
	os.Exit(0)
}

// // checkPidFile checks the configured pid file for a currently running process
// // If the process is still running, fail the execution.
// func checkPidFile() {
// 	fpPid, err := os.Open(GetPidFile())
// 	if err != nil {
// 		// File probably doesn't exist, should check error code.
// 		if os.IsNotExist(err) {
// 			log.Println("Pid file does not exist, proceed with creation.")
// 			return
// 		} else {
// 			log.Fatalf("Got an error trying to open pid file %s\n", err.Error())
// 		}
// 	}
// 	var oldPid int
// 	fmt.Fscanf(fpPid, "%d", &oldPid)

// 	log.Printf("check if old pid is still running: %d\n", oldPid)
// 	proc, err := os.FindProcess(oldPid)

// 	if err != nil {
// 		log.Printf("Problem trying to find old process: %+v", err)
// 		return
// 	}

// 	err = proc.Signal(syscall.Signal(0))

// 	if err == nil {
// 		log.Fatal(errors.New("Process still active, do not start."))
// 	}

// 	err = os.Remove(GetPidFile())
// 	if err != nil {
// 		log.Printf("Remove pid file, error return %+v", err)
// 	}

// 	log.Printf("Removed %s.", GetPidFile())
// }

// if it exists already, panic/log fatal. The run script should check for a pid file,
// if it is still running, abort starting. If it is not running, delete the file before starting.
// in the goroutine, register sigusr1 maybe as a restart rather than a stop.
// when restarting, todo.
// If the pid file exists and go is not running, that means it crashed somehow.
// func setPidFile() {
// 	pid_handle, err := os.OpenFile(GetPidFile(), os.O_WRONLY|os.O_CREATE|os.O_EXCL, 0600)
// 	if err != nil {
// 		log.Fatal(err)
// 	}
// 	defer pid_handle.Close()
// 	_, err = fmt.Fprintf(pid_handle, "%d", os.Getpid())

// 	if err != nil {
// 		defer os.Remove(GetPidFile())
// 		log.Fatal(err)
// 	}
// }
