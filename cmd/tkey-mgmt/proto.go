// Copyright (C) 2022, 2023 - Tillitis AB
// SPDX-License-Identifier: GPL-2.0-only

package main

import (
	"fmt"

	"github.com/tillitis/tkeyclient"
	"github.com/tillitis/tkeyutil"

	"golang.org/x/crypto/blake2s"
)

var (
	cmdGetNameVersion    = appCmd{0x01, "cmdGetNameVersion", tkeyclient.CmdLen1}
	rspGetNameVersion    = appCmd{0x02, "rspGetNameVersion", tkeyclient.CmdLen32}
	cmdLoadApp           = appCmd{0x03, "cmdLoadApp", tkeyclient.CmdLen128}
	rspLoadApp           = appCmd{0x04, "rspLoadApp", tkeyclient.CmdLen4}
	cmdLoadAppData       = appCmd{0x05, "cmdLoadAppData", tkeyclient.CmdLen128}
	rspLoadAppData       = appCmd{0x06, "rspLoadAppData", tkeyclient.CmdLen4}
	rspLoadAppDataReady  = appCmd{0x07, "rspLoadAppDataReady", tkeyclient.CmdLen128}
	cmdDeleteApp         = appCmd{0x08, "cmdDeleteApp", tkeyclient.CmdLen1}
	rspDeleteApp         = appCmd{0x09, "rspDeleteApp", tkeyclient.CmdLen4}
	cmdRegisterMgmtApp   = appCmd{0x0a, "cmdRegisterMgmtApp", tkeyclient.CmdLen1}
	rspRegisterMgmtApp   = appCmd{0x0b, "rspRegisterMgmtApp", tkeyclient.CmdLen4}
	cmdUnregisterMgmtApp = appCmd{0x0c, "cmdUnregisterMgmtApp", tkeyclient.CmdLen1}
	rspUnregisterMgmtApp = appCmd{0x0d, "rspUnregisterMgmtApp", tkeyclient.CmdLen4}

	cmdLoadAppFlash = appCmd{0xf0, "cmdGetNameVersion", tkeyclient.CmdLen1}
	rspLoadAppFlash = appCmd{0xf1, "rspGetNameVersion", tkeyclient.CmdLen4}

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

func LoadMgmtApp(devPath string, speed int, fileUSS string, enterUSS bool) (*Mgmt, error) {
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

	mgmtApp := New(tk)

	if !isWantedApp(mgmtApp) {
		mgmtApp.Close()
		return nil, fmt.Errorf("no TKey on the serial port, or it's running wrong app (and is not in firmware mode)")
	}

	return &mgmtApp, nil
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

// DeleteInstalledApp sends a command to delete an already installed app, if any
func (m Mgmt) DeleteInstalledApp() error {

	id := 2
	tx, err := tkeyclient.NewFrameBuf(cmdDeleteApp, id)
	if err != nil {
		return err
	}

	tkeyclient.Dump("DeleteApp tx", tx)

	if err = m.tk.Write(tx); err != nil {
		return err
	}

	// Wait for reply
	rx, _, err := m.tk.ReadFrame(rspDeleteApp, id)
	if err != nil {
		return fmt.Errorf("ReadFrame: %w", err)
	}

	if rx[2] != tkeyclient.StatusOK {
		return fmt.Errorf("DeleteApp NOK")
	}

	return nil
}

// RegisterMgmtApp sends a command to register the already loaded app as a management app
func (m Mgmt) RegisterMgmtApp(unregister bool) error {

	var cmd, rsp appCmd
	if unregister {
		cmd = cmdUnregisterMgmtApp
		rsp = rspUnregisterMgmtApp
	} else {
		cmd = cmdRegisterMgmtApp
		rsp = rspRegisterMgmtApp
	}
	id := 2
	tx, err := tkeyclient.NewFrameBuf(cmd, id)
	if err != nil {
		return err
	}

	tkeyclient.Dump("RegisterMgmtApp tx", tx)

	if err = m.tk.Write(tx); err != nil {
		return err
	}

	// Wait for reply
	rx, _, err := m.tk.ReadFrame(rsp, id)
	if err != nil {
		return fmt.Errorf("ReadFrame: %w", err)
	}

	if rx[2] != tkeyclient.StatusOK {
		return fmt.Errorf("RegisterMgmtApp NOK")
	}

	return nil
}

func (m Mgmt) InstallApp(bin []byte, secretPhrase []byte) error {

	binLen := len(bin)
	if binLen > 100*1024 { // TK1_APP_MAX_SIZE
		return fmt.Errorf("File too big")
	}

	if verbose {
		le.Printf("app size: %v, 0x%x, 0b%b\n", binLen, binLen, binLen)

	}

	err := m.installApp(binLen, secretPhrase)
	if err != nil {
		return err
	}

	// Load the file
	var offset int
	var deviceDigest [32]byte

	for nsent := 0; offset < binLen; offset += nsent {
		if binLen-offset <= cmdLoadAppData.CmdLen().Bytelen()-1 {
			deviceDigest, nsent, err = m.installAppData(bin[offset:], true)
		} else {
			_, nsent, err = m.installAppData(bin[offset:], false)
		}
		if err != nil {
			return fmt.Errorf("InstallApp: %w", err)
		}
	}
	if offset > binLen {
		return fmt.Errorf("transmitted more than expected")
	}

	// TODO: Add checking of the digest

	// digest := blake2s.Sum256(bin)
	//
	// le.Printf("Digest from host:\n")
	// printDigest(digest)
	le.Printf("Digest from device:\n")
	printDigest(deviceDigest)
	//
	// if deviceDigest != digest {
	// 	return fmt.Errorf("Different digests")
	// }
	// le.Printf("Same digests!\n")

	// The app has now started automatically.
	return nil
}

// loadApp sets the size and USS of the app to be loaded into the TKey.
func (m Mgmt) installApp(size int, secretPhrase []byte) error {
	id := 2
	tx, err := tkeyclient.NewFrameBuf(cmdLoadApp, id)
	if err != nil {
		return err
	}

	// Set size
	tx[2] = byte(size)
	tx[3] = byte(size >> 8)
	tx[4] = byte(size >> 16)
	tx[5] = byte(size >> 24)

	if len(secretPhrase) == 0 {
		tx[6] = 0
	} else {
		tx[6] = 1
		// Hash user's phrase as USS
		uss := blake2s.Sum256(secretPhrase)
		copy(tx[6:], uss[:])
	}

	tkeyclient.Dump("installApp tx", tx)
	if err = m.tk.Write(tx); err != nil {
		return err
	}

	rx, _, err := m.tk.ReadFrame(rspLoadApp, id)
	if err != nil {
		return fmt.Errorf("ReadFrame: %w", err)
	}
	tkeyclient.Dump("installApp rx", rx)

	if rx[2] != tkeyclient.StatusOK {
		return fmt.Errorf("installApp NOK")
	}

	return nil
}

// loadAppData loads a chunk of the raw app binary into the TKey.
func (m Mgmt) installAppData(content []byte, last bool) ([32]byte, int, error) {
	id := 2
	tx, err := tkeyclient.NewFrameBuf(cmdLoadAppData, id)
	if err != nil {
		return [32]byte{}, 0, err
	}

	payload := make([]byte, cmdLoadAppData.CmdLen().Bytelen()-1)
	copied := copy(payload, content)

	// Add padding if not filling the payload buffer.
	if copied < len(payload) {
		padding := make([]byte, len(payload)-copied)
		copy(payload[copied:], padding)
	}

	copy(tx[2:], payload)

	tkeyclient.Dump("installAppData tx", tx)

	if err = m.tk.Write(tx); err != nil {
		return [32]byte{}, 0, err
	}

	var rx []byte
	var expectedResp appCmd

	if last {
		expectedResp = rspLoadAppDataReady
	} else {
		expectedResp = rspLoadAppData
	}

	// Wait for reply
	rx, _, err = m.tk.ReadFrame(expectedResp, id)
	if err != nil {
		return [32]byte{}, 0, fmt.Errorf("ReadFrame: %w", err)
	}

	tkeyclient.Dump("installAppData rx", rx)

	if rx[2] != tkeyclient.StatusOK {
		return [32]byte{}, 0, fmt.Errorf("installAppData NOK")
	}

	if last {
		var digest [32]byte
		copy(digest[:], rx[3:])
		return digest, copied, nil
	}

	return [32]byte{}, copied, nil
}

func printDigest(md [32]byte) {
	digest := ""
	for j := 0; j < 4; j++ {
		for i := 0; i < 8; i++ {
			digest += fmt.Sprintf("%02x", md[i+8*j])
		}
		digest += " "
	}
	le.Printf(digest + "\n")
}
