package main

import (
	"fmt"
	"io"
	"os"
	"reflect"
	"strconv"
	"time"

	"go.bug.st/serial"
)

// TillitisKey is a serial connection to a TKey and the commands that
// the firmware supports.
type TillitisKey struct {
	speed int
	conn  serial.Port
}

var length = 10

func main() {

	if len(os.Args) != 3 {
		fmt.Printf("Include port and length\n")
		os.Exit(1)
	}
	// var port = "/dev/tty.usbmodem11101"
	var port string
	port = os.Args[1]
	length, _ = strconv.Atoi(os.Args[2])
	var speed = 62500
	tk := &TillitisKey{}
	var err error

	tk.conn, err = serial.Open(port, &serial.Mode{BaudRate: speed})
	if err != nil {
		fmt.Printf("Error serial open: %w", err)
	}
	defer tk.conn.Close()

	tx := make([]byte, length)
	rx := make([]byte, length)
	for i := 0; i < length; i++ {
		tx[i] = byte(i)
		fmt.Printf("%d", tx[i])
	}
	fmt.Printf("\n")

	tk.SetReadTimeout(2)

	tk.write(tx)

	var n int
	rx, n, err = tk.ReadFrame()
	if err != nil {
		fmt.Printf("Error read: %w", err)
	}

	for i := 0; i < n; i++ {
		fmt.Printf("%d", rx[i])
	}
	fmt.Printf("\n")

	equal := reflect.DeepEqual(tx, rx)
	fmt.Printf("Equal %t\n", equal)

}

func (tk TillitisKey) write(d []byte) error {
	_, err := tk.conn.Write(d)
	if err != nil {
		return fmt.Errorf("write: %w", err)
	}

	fmt.Printf("write\n")
	return nil
}

func (tk TillitisKey) ReadFrame() ([]byte, int, error) {
	rx := make([]byte, length)
	n, err := io.ReadFull(tk.conn, rx)
	// var i = 0
	// var n int
	// var err error
	// for {

	// 	n, err = tk.conn.Read(rx)
	// 	if err != nil {
	// 		fmt.Printf("Error read: %w", err)

	// 	}
	// 	i += n
	// 	if i >= length {
	// 		break
	// 	}

	// }

	fmt.Printf("Read %d bytes\n", n)

	return rx, n, err
}

func (tk TillitisKey) SetReadTimeout(seconds int) error {
	var t time.Duration = -1
	if seconds > 0 {
		t = time.Duration(seconds) * time.Second
	}
	if err := tk.conn.SetReadTimeout(t); err != nil {
		return fmt.Errorf("SetReadTimeout: %w", err)
	}
	return nil
}
