// Copyright (C) 2022, 2023 - Tillitis AB
// SPDX-License-Identifier: GPL-2.0-only

package main

import (
	"fmt"

	"github.com/tillitis/tkeyclient"
	"github.com/tillitis/tkeyutil"
)

var (
	cmdGetNameVersion    = appCmd{0x01, "cmdGetNameVersion", tkeyclient.CmdLen1}
	rspGetNameVersion    = appCmd{0x02, "rspGetNameVersion", tkeyclient.CmdLen32}
	cmdLoadApp           = appCmd{0x03, "cmdLoadApp", tkeyclient.CmdLen4}
	rspLoadApp           = appCmd{0x04, "rspLoadApp", tkeyclient.CmdLen4}
	cmdLoadAppData       = appCmd{0x05, "cmdLoadAppData", tkeyclient.CmdLen128}
	rspLoadAppData       = appCmd{0x06, "rspLoadAppData", tkeyclient.CmdLen4}
	cmdLoadAppDataReady  = appCmd{0x07, "cmdLoadAppDataReady", tkeyclient.CmdLen128}
	cmdLoadAppFlash      = appCmd{0xf0, "cmdGetNameVersion", tkeyclient.CmdLen1}
	rspLoadAppFlash      = appCmd{0xf1, "rspGetNameVersion", tkeyclient.CmdLen1}
	cmdUnregisterMgmtApp = appCmd{0xf2, "cmdLoadApp", tkeyclient.CmdLen1}
	rspUnregisterMgmtApp = appCmd{0xf3, "rspLoadApp", tkeyclient.CmdLen1}

	// Commands to fw
	cmdStartAppFlash = fwCmd{0xF0, "cmdStartAppFlash", tkeyclient.CmdLen1}
	rspStartAppFlash = fwCmd{0xF1, "rspStartAppFlash", tkeyclient.CmdLen1}
)

type appCmd struct {
	code   byte
	name   string
	cmdLen tkeyclient.CmdLen
}

func (c appCmd) Code() byte {
	return c.code
}

func (c appCmd) CmdLen() tkeyclient.CmdLen {
	return c.cmdLen
}

func (c appCmd) Endpoint() tkeyclient.Endpoint {
	return tkeyclient.DestApp
}

func (c appCmd) String() string {
	return c.name
}

type fwCmd struct {
	code   byte
	name   string
	cmdLen tkeyclient.CmdLen
}

func (c fwCmd) Code() byte {
	return c.code
}

func (c fwCmd) CmdLen() tkeyclient.CmdLen {
	return c.cmdLen
}

func (c fwCmd) Endpoint() tkeyclient.Endpoint {
	return tkeyclient.DestFW
}

func (c fwCmd) String() string {
	return c.name
}

type Mgmt struct {
	tk *tkeyclient.TillitisKey // A connection to a TKey
}

// New allocates a struct for communicating with the random app
// running on the TKey. You're expected to pass an existing connection
// to it, so use it like this:
//
//	tk := tkeyclient.New()
//	err := tk.Connect(port)
//	randomGen := New(tk)
func New(tk *tkeyclient.TillitisKey) Mgmt {
	var mgmt Mgmt

	mgmt.tk = tk

	return mgmt
}

// Close closes the connection to the TKey
func (m Mgmt) Close() error {
	if err := m.tk.Close(); err != nil {
		return fmt.Errorf("tk.Close: %w", err)
	}
	return nil
}

// GetAppNameVersion gets the name and version of the running app in
// the same style as the stick itself.
func (m Mgmt) GetAppNameVersion() (*tkeyclient.NameVersion, error) {
	id := 2
	tx, err := tkeyclient.NewFrameBuf(cmdGetNameVersion, id)
	if err != nil {
		return nil, fmt.Errorf("NewFrameBuf: %w", err)
	}

	tkeyclient.Dump("GetAppNameVersion tx", tx)
	if err = m.tk.Write(tx); err != nil {
		return nil, fmt.Errorf("Write: %w", err)
	}

	err = m.tk.SetReadTimeout(2)
	if err != nil {
		return nil, fmt.Errorf("SetReadTimeout: %w", err)
	}

	rx, _, err := m.tk.ReadFrame(rspGetNameVersion, id)
	if err != nil {
		return nil, fmt.Errorf("ReadFrame: %w", err)
	}

	err = m.tk.SetReadTimeout(0)
	if err != nil {
		return nil, fmt.Errorf("SetReadTimeout: %w", err)
	}

	nameVer := &tkeyclient.NameVersion{}
	nameVer.Unpack(rx[2:])

	return nameVer, nil
}

func LoadMgmtApp(devPath string, speed int, fileUSS string, enterUSS bool) (*tkeyclient.TillitisKey, error) {
	if !verbose {
		tkeyclient.SilenceLogging()
	}

	if devPath == "" {
		var err error
		devPath, err = tkeyclient.DetectSerialPort(false)
		if err != nil {
			return nil, fmt.Errorf("DetectSerialPort: %w", err)
		}
	}

	tk := tkeyclient.New()
	if verbose {
		le.Printf("Connecting to TKey on serial port %s ...", devPath)
	}
	if err := tk.Connect(devPath, tkeyclient.WithSpeed(speed)); err != nil {
		return nil, fmt.Errorf("could not open %s: %w", devPath, err)
	}

	if isFirmwareMode(tk) {
		var secret []byte
		var err error

		if enterUSS {
			secret, err = tkeyutil.InputUSS()
			if err != nil {
				tk.Close()
				return nil, fmt.Errorf("InputUSS: %w", err)
			}
		}
		if fileUSS != "" {
			secret, err = tkeyutil.ReadUSS(fileUSS)
			if err != nil {
				tk.Close()
				return nil, fmt.Errorf("ReadUSS: %w", err)
			}
		}

		if err := tk.LoadApp(mgmtBinary, secret); err != nil {
			tk.Close()
			return nil, fmt.Errorf("could not load management app: %w", err)
		}

		if verbose {
			le.Printf("Management app loaded.")
		}
	} else {
		if enterUSS || fileUSS != "" {
			le.Printf("WARNING: App already loaded, your USS won't be used.")
		} else {
			le.Printf("WARNING: App already loaded.")
		}
	}

	return tk, nil
}

// StartAppFlash sends a command to fw to start the preloaded app, if any
func StartAppFlash(devPath string, speed int) error {

	if !verbose {
		tkeyclient.SilenceLogging()
	}

	if devPath == "" {
		var err error
		devPath, err = tkeyclient.DetectSerialPort(false)
		if err != nil {
			return fmt.Errorf("DetectSerialPort: %w", err)
		}
	}

	tk := tkeyclient.New()
	if verbose {
		le.Printf("Connecting to TKey on serial port %s ...", devPath)
	}
	if err := tk.Connect(devPath, tkeyclient.WithSpeed(speed)); err != nil {
		return fmt.Errorf("could not open %s: %w", devPath, err)
	}
	defer tk.Close()

	nameVer, err := tk.GetNameVersion()
	if err != nil {
		le.Printf("GetNameVersion failed: %v\n", err)
		le.Printf("If the serial port is correct, then the TKey might not be in firmware-\n" +
			"mode, and have an app running already. Please unplug and plug it in again.\n")
		return fmt.Errorf("GetNameVersion failed: %w\n", err)
	}
	le.Printf("Firmware name0:'%s' name1:'%s' version:%d\n",
		nameVer.Name0, nameVer.Name1, nameVer.Version)

	id := 2
	tx, err := tkeyclient.NewFrameBuf(cmdStartAppFlash, id)
	if err != nil {
		return err
	}

	tkeyclient.Dump("StartAppFlash tx", tx)
	if err = tk.Write(tx); err != nil {
		return err
	}

	if err = tk.SetReadTimeout(2); err != nil {
		return err
	}

	_, _, err = tk.ReadFrame(rspStartAppFlash, id)
	if err != nil {
		return fmt.Errorf("ReadFrame: %w", err)
	}

	if err = tk.SetReadTimeout(0); err != nil {
		return fmt.Errorf("SetReadTimeout: %w", err)
	}

	return nil
}
