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

type cmd struct {
	cmd  int
	name string
}

var (
	cmdStart   = cmd{0, "start"}
	cmdInstall = cmd{1, "install"}
)

func main() {
	var appPath, devPath, fileUSS string
	var speed int
	var enterUSS, helpOnly, overwrite bool

	if version == "" {
		version = readBuildInfo()
	}

	// Default flags to show
	pflag.CommandLine.SetOutput(os.Stderr)
	root := pflag.NewFlagSet("root", pflag.ExitOnError)
	root.BoolVar(&verbose, "verbose", false, "Enable verbose output.")
	versionOnly := root.BoolP("version", "v", false, "Output version information.")
	root.BoolVarP(&helpOnly, "help", "h", false, "Output this help.")
	root.Usage = func() {
		desc := fmt.Sprintf(`%[1]s is a WIP Client Management
app for the Tillitis' TKey.

Usage:
  %[1]s <command> [flags] FILE...

Commands:
  start       Starts an already installed app
  install     Verify signature of previously generated data

Use <command> --help for further help, i.e. %[1]s verify --help

Flags:`, os.Args[0])
		le.Printf("%s\n%s", desc,
			root.FlagUsagesWrapped(86))
	}

	// Flags for the command cmdStart
	cmdStartFlags := pflag.NewFlagSet(cmdStart.name, pflag.ExitOnError)
	cmdStartFlags.SortFlags = false
	cmdStartFlags.StringVar(&devPath, "port", "",
		"Set serial port device `PATH`. If this is not passed, auto-detection will be attempted.")
	cmdStartFlags.IntVar(&speed, "speed", tkeyclient.SerialSpeed,
		"Set serial port speed in `BPS` (bits per second).")
	cmdStartFlags.BoolVar(&helpOnly, "help", false, "Output this help.")
	cmdStartFlags.Usage = func() {
		desc := fmt.Sprintf(`Usage: %[1]s [flags...] 

Sends a command to the connected TKey to start the preloaded app.`, os.Args[0])
		le.Printf("%s\n\n%s", desc,
			cmdStartFlags.FlagUsagesWrapped(86))
	}

	// Flags for the command cmdInstall
	cmdInstallFlags := pflag.NewFlagSet(cmdInstall.name, pflag.ExitOnError)
	cmdInstallFlags.SortFlags = false
	cmdInstallFlags.BoolVar(&overwrite, "overwrite", false,
		"Overwrite the already installed app.")
	cmdInstallFlags.StringVar(&devPath, "port", "",
		"Set serial port device `PATH`. If this is not passed, auto-detection will be attempted.")
	cmdInstallFlags.IntVar(&speed, "speed", tkeyclient.SerialSpeed,
		"Set serial port speed in `BPS` (bits per second).")
	cmdInstallFlags.BoolVar(&enterUSS, "uss", false,
		"Enable typing of a phrase to be hashed as the User Supplied Secret. The USS is loaded onto the TKey along with the app itself and used by the firmware, together with other material, for deriving secrets for the application.")
	cmdInstallFlags.StringVar(&fileUSS, "uss-file", "",
		"Read `FILE` and hash its contents as the USS. Use '-' (dash) to read from stdin. The full contents are hashed unmodified (e.g. newlines are not stripped).")
	cmdInstallFlags.BoolVar(&verbose, "verbose", false, "Enable verbose output.")
	cmdInstallFlags.BoolVar(&helpOnly, "help", false, "Output this help.")
	cmdInstallFlags.Usage = func() {
		desc := fmt.Sprintf(`Usage: %[1]s [flags...] <app> 

Installs the specified app on the connected TKey.`, os.Args[0])
		le.Printf("%s\n\n%s", desc,
			cmdInstallFlags.FlagUsagesWrapped(86))
	}

	// No arguments, print and exit
	if len(os.Args) == 1 {
		root.Usage()
		os.Exit(2)
	}

	// version? Print and exit
	if len(os.Args) == 2 {
		if err := root.Parse(os.Args); err != nil {
			le.Printf("Error parsing input arguments: %v\n", err)
			os.Exit(2)
		}
		if *versionOnly {
			le.Printf("tkey-mgmt %s", version)
			os.Exit(0)
		}
		if helpOnly {
			root.Usage()
			os.Exit(0)
		}

	}

	switch os.Args[1] {
	case cmdStart.name:
		if err := cmdStartFlags.Parse(os.Args[2:]); err != nil {
			le.Printf("Error parsing input arguments: %v\n", err)
			os.Exit(2)
		}

		if helpOnly {
			cmdStartFlags.Usage()
			os.Exit(0)
		}

		if cmdStartFlags.NArg() > 1 {
			le.Printf("Unexpected argument: %s\n\n", strings.Join(os.Args[2:], " "))
			cmdStartFlags.Usage()
			os.Exit(2)
		}

		err := StartAppFlash(devPath, speed)
		if err != nil {
			le.Printf("StartAppFlash failed: %v\n", err)
			os.Exit(1)
		}

		le.Printf("App started from flash\n")

		os.Exit(0)
	case cmdInstall.name:
		if err := cmdInstallFlags.Parse(os.Args[2:]); err != nil {
			le.Printf("Error parsing input arguments: %v\n", err)
			os.Exit(2)
		}

		if helpOnly {
			cmdInstallFlags.Usage()
			os.Exit(0)
		}

		if cmdInstallFlags.NArg() < 1 {
			le.Printf("Missing path to app.\n\n")
			cmdInstallFlags.Usage()
			os.Exit(2)
		} else if cmdInstallFlags.NArg() > 1 {
			le.Printf("Unexpected argument: %s\n\n", strings.Join(cmdInstallFlags.Args()[1:], " "))
			cmdInstallFlags.Usage()
			os.Exit(2)
		}

		appPath = cmdInstallFlags.Args()[0]
		fmt.Printf("appPath: %v\n", appPath)

		m, err := LoadMgmtApp(devPath, speed, fileUSS, enterUSS)
		if err != nil {
			le.Printf("Error: LoadMgmtApp: %v\n", err)
			os.Exit(1)
		}

		appBin, err := os.ReadFile(appPath)
		if err != nil {
			le.Printf("Failed to read file: %v\n", err)
			os.Exit(1)
		}

		// TODO: Add uss handling
		err = m.InstallApp([]byte(appBin), nil)
		if err != nil {
			le.Printf("Error: InstallApp: %v\n", err)
			le.Printf("Failed to install app, either not a registered Management app, or there is already an installed app.\n")
			os.Exit(1)
		}

		os.Exit(0)
	default:
		root.Usage()
		le.Printf("%q is not a valid subcommand.\n", os.Args[1])
		os.Exit(2)
	}
	os.Exit(1) // should never be reached
}
