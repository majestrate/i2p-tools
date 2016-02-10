package sam3

import (
	"fmt"
	"testing"
	"time"
)

const yoursam = "127.0.0.1:7656"

func Test_Basic(t *testing.T) {
	fmt.Println("Test_Basic")
	fmt.Println("\tAttaching to SAM at " + yoursam)
	sam, err := NewSAM(yoursam)
	if err != nil {
		fmt.Println(err.Error)
		t.Fail()
		return
	}

	fmt.Println("\tCreating new keys...")
	keys, err := sam.NewKeys()
	if err != nil {
		fmt.Println(err.Error())
		t.Fail()
	} else {
		fmt.Println("\tAddress created: " + keys.Addr().Base32())
		fmt.Println("\tI2PKeys: " + string(keys.both)[:50] + "(...etc)")
	}

	addr2, err := sam.Lookup("zzz.i2p")
	if err != nil {
		fmt.Println(err.Error())
		t.Fail()
	} else {
		fmt.Println("\tzzz.i2p = " + addr2.Base32())
	}

	if err := sam.Close(); err != nil {
		fmt.Println(err.Error())
		t.Fail()
	}
}

/*
func Test_GenericSession(t *testing.T) {
	if testing.Short() {
		return
	}
	fmt.Println("Test_GenericSession")
	sam, err := NewSAM(yoursam)
	if err != nil {
		fmt.Println(err.Error)
		t.Fail()
		return
	}
	keys, err := sam.NewKeys()
	if err != nil {
		fmt.Println(err.Error())
		t.Fail()
	} else {
		conn1, err := sam.newGenericSession("STREAM", "testTun", keys, []string{})
		if err != nil {
			fmt.Println(err.Error())
			t.Fail()
		} else {
			conn1.Close()
		}
		conn2, err := sam.newGenericSession("STREAM", "testTun", keys, []string{"inbound.length=1", "outbound.length=1", "inbound.lengthVariance=1", "outbound.lengthVariance=1", "inbound.quantity=1", "outbound.quantity=1"})
		if err != nil {
			fmt.Println(err.Error())
			t.Fail()
		} else {
			conn2.Close()
		}
		conn3, err := sam.newGenericSession("DATAGRAM", "testTun", keys, []string{"inbound.length=1", "outbound.length=1", "inbound.lengthVariance=1", "outbound.lengthVariance=1", "inbound.quantity=1", "outbound.quantity=1"})
		if err != nil {
			fmt.Println(err.Error())
			t.Fail()
		} else {
			conn3.Close()
		}
	}
	if err := sam.Close(); err != nil {
		fmt.Println(err.Error())
		t.Fail()
	}
}
*/

func Test_RawServerClient(t *testing.T) {
	if testing.Short() {
		return
	}

	fmt.Println("Test_RawServerClient")
	sam, err := NewSAM(yoursam)
	if err != nil {
		t.Fail()
		return
	}
	defer sam.Close()
	keys, err := sam.NewKeys()
	if err != nil {
		t.Fail()
		return
	}
	fmt.Println("\tServer: Creating tunnel")
	rs, err := sam.NewDatagramSession("RAWserverTun", keys, []string{"inbound.length=0", "outbound.length=0", "inbound.lengthVariance=0", "outbound.lengthVariance=0", "inbound.quantity=1", "outbound.quantity=1"}, 0)
	if err != nil {
		fmt.Println("Server: Failed to create tunnel: " + err.Error())
		t.Fail()
		return
	}
	c, w := make(chan bool), make(chan bool)
	go func(c, w chan (bool)) {
		sam2, err := NewSAM(yoursam)
		if err != nil {
			c <- false
			return
		}
		defer sam2.Close()
		keys, err := sam2.NewKeys()
		if err != nil {
			c <- false
			return
		}
		fmt.Println("\tClient: Creating tunnel")
		rs2, err := sam2.NewDatagramSession("RAWclientTun", keys, []string{"inbound.length=0", "outbound.length=0", "inbound.lengthVariance=0", "outbound.lengthVariance=0", "inbound.quantity=1", "outbound.quantity=1"}, 0)
		if err != nil {
			c <- false
			return
		}
		defer rs2.Close()
		fmt.Println("\tClient: Tries to send raw datagram to server")
		for {
			select {
			default:
				_, err = rs2.WriteTo([]byte("Hello raw-world! <3 <3 <3 <3 <3 <3"), rs.LocalAddr())
				if err != nil {
					fmt.Println("\tClient: Failed to send raw datagram: " + err.Error())
					c <- false
					return
				}
				time.Sleep(5 * time.Second)
			case <-w:
				fmt.Println("\tClient: Sent raw datagram, quitting.")
				return
			}
		}
		c <- true
	}(c, w)
	buf := make([]byte, 512)
	fmt.Println("\tServer: Read() waiting...")
	n, _, err := rs.ReadFrom(buf)
	w <- true
	if err != nil {
		fmt.Println("\tServer: Failed to Read(): " + err.Error())
		t.Fail()
		return
	}
	fmt.Println("\tServer: Received datagram: " + string(buf[:n]))
	//	fmt.Println("\tServer: Senders address was: " + saddr.Base32())
}
