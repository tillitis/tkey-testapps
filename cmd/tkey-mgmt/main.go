// Copyright (C) 2022, 2023 - Tillitis AB
// SPDX-License-Identifier: GPL-2.0-only

package main

import (
	_ "embed"
	"fmt"
	"log"
	"os"
	"strings"

	"github.com/spf13/pflag"
	"github.com/tillitis/tkeyclient"
)

//go:embed app.bin
var mgmtBinary []byte

// Use when printing err/diag msgs
var le = log.New(os.Stderr, "", 0)

var (
	version string
	verbose = false
)

func main() {
	var fileName, devPath, fileUSS, preloadApp string
	var speed int
	var enterUSS, helpOnly, startFromFlash bool
	pflag.CommandLine.SetOutput(os.Stderr)
	pflag.CommandLine.SortFlags = false
	pflag.StringVar(&devPath, "port", "",
		"Set serial port device `PATH`. If this is not passed, auto-detection will be attempted.")
	pflag.IntVar(&speed, "speed", tkeyclient.SerialSpeed,
		"Set serial port speed in `BPS` (bits per second).")
	pflag.BoolVar(&enterUSS, "uss", false,
		"Enable typing of a phrase to be hashed as the User Supplied Secret. The USS is loaded onto the TKey along with the app itself and used by the firmware, together with other material, for deriving secrets for the application.")
	pflag.StringVar(&fileUSS, "uss-file", "",
		"Read `FILE` and hash its contents as the USS. Use '-' (dash) to read from stdin. The full contents are hashed unmodified (e.g. newlines are not stripped).")
	pflag.StringVar(&preloadApp, "preload-app", "",
		"The `APP` to store in flash, as a preloaded app.")
	pflag.BoolVar(&startFromFlash, "start", false, "Start a preloaded app in flash.")
	pflag.BoolVar(&verbose, "verbose", false, "Enable verbose output.")
	pflag.BoolVar(&helpOnly, "help", false, "Output this help.")
	versionOnly := pflag.BoolP("version", "v", false, "Output version information.")
	pflag.Usage = func() {
		desc := fmt.Sprintf(`Usage: %[1]s [flags...] FILE

%[1]s loads an application binary from FILE onto Tillitis TKey
and starts it.

Exit status code is 0 if the app is both successfully loaded and started. Exit
code is non-zero if anything goes wrong, for example if TKey is already
running some app.`, os.Args[0])
		le.Printf("%s\n\n%s", desc,
			pflag.CommandLine.FlagUsagesWrapped(86))
	}
	pflag.Parse()

	if version == "" {
		version = readBuildInfo()
	}

	if pflag.NArg() > 0 {
		if pflag.NArg() > 1 {
			le.Printf("Unexpected argument: %s\n\n", strings.Join(pflag.Args()[1:], " "))
			pflag.Usage()
			os.Exit(2)
		}
		fileName = pflag.Args()[0]
	}

	if *versionOnly {
		le.Printf("tkey-mgmt %s", version)
		os.Exit(0)
	}

	if helpOnly {
		pflag.Usage()
		os.Exit(0)
	}

	if fileName == "" {
		le.Printf("Please pass an app binary FILE.\n\n")
	}

	if startFromFlash {
		err := StartAppFlash(devPath, speed)
		if err != nil {
			le.Printf("StartAppFlash failed: %v\n", err)
			os.Exit(1)
		}

		le.Printf("App started from flash\n")
		os.Exit(0)
	}

	_, err := LoadMgmtApp(devPath, speed, fileUSS, enterUSS)
	if err != nil {
		le.Printf("Error: loadMgmtApp: %v", err)
		os.Exit(1)
	}

	os.Exit(0)

}
