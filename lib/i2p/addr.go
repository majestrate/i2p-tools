package i2p

import (
	"github.com/majestrate/i2p-tools/sam3"
)

type B32Addr sam3.I2PDestHash

func (a B32Addr) String() (str string) {
	str = sam3.I2PDestHash(a).String()
	return
}
