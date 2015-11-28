//
// config.go -- samtun configuration
//
package samtun

import (
  "encoding/json"
  "io/ioutil"
)

// options passed to sam
type samOpts map[string]string

func (opts samOpts) List() (ls []string) {
    for k, v := range opts {
        ls = append(ls, k+"="+v)
    }
    return
}

// sam config parameters
type samConfig struct {
    KeyFile string // file to private key
    Addr string // sam interface address
    Session string // sam session name
    Opts samOpts // sam session options
}

// config for exit policy, traffic shaping, port/address white/black listing
type exitPolicyConfig struct {
    BlacklistFile string // file for blacklist rules
    WhitelistFile string // file for whitelist rules
    BlockPorts []int // explicitly block these ports for outbound
    AllowPorts []int // explicitly allow these ports for outbound
}

// exit network config
type exitNetConfig struct {
    NetName string // name for network
    Sam samConfig // sam config
    IfName string // network interface name
    IfAddr string // network interface address
    IfMask string // network interface netmask
    IfMTU int // network interface mtu
    Policy exitPolicyConfig // exit policy
}

// config for exit server
type exitConfig struct {
    ConfigVersion int // version number
    Exits []exitNetConfig // multiple exits
    Exit exitNetConfig // default exit
}

// config for layer 2 link to hub
type linkConfig struct {
    IfName string // with interface to bind to
    AdvertiseInterval int // how many milliseconds between avertisement beacons
}

// config for hub dhcp
type dhcpConfig struct {
    Addr string
}

// lan connectivity config for hub
type lanConfig struct {
    DHCP dhcpConfig 
    LinkKeyFile string // link level private key file
    Links []linkConfig // layer 2 links
}

// config for hub
type hubConfig struct {
    ConfigVersion int
    Sam samConfig
    ExitDest string
    Lan lanConfig
}

func (cfg *exitConfig) Save(fname string) (err error) {
    return saveJson(cfg, fname)
}

func (cfg *hubConfig) Save(fname string) (err error) {
    return saveJson(cfg, fname)
}

func genDefaultSamOpts(exit bool) (opts samOpts) {
    opts = make(samOpts)
    hopcount := "3"
    if exit {
        hopcount = "0"
    }
    opts["inbound.length"] = hopcount
    opts["outbound.length"] = hopcount
    return
}

func genDefaultSamConfig(exit bool) (cfg samConfig) {
    cfg.KeyFile = "i2p-priv.key"
    cfg.Addr = "127.0.0.1:7656"
    cfg.Session = "i2tun"
    cfg.Opts = genDefaultSamOpts(exit)
    return
}

func genDefaultExitPolicyConfig() (policy exitPolicyConfig) {
    policy.BlacklistFile = "blacklist.conf"
    policy.WhitelistFile = "whitelist.conf"
    policy.BlockPorts = []int{80, 23, 25}
    policy.AllowPorts = []int{443, 22}
    return
}

func genDefaultExitNet() (exit exitNetConfig) {
    exit.Sam = genDefaultSamConfig(true)
    exit.NetName = "NetworkOfDoom"
    exit.IfName = "exit0"
    exit.IfAddr = "10.55.0.1"
    exit.IfMask = "255.255.255.0"
    exit.Policy = genDefaultExitPolicyConfig()
    return
}

func genDefaultDHCPConfig() (cfg dhcpConfig) {
    cfg.Addr = "0.0.0.0:68"
    return
}

func genLanConfig() (cfg lanConfig) {
    cfg.DHCP = genDefaultDHCPConfig()
    cfg.LinkKeyFile = "lan-priv.key"
    cfg.Links = []linkConfig{
        linkConfig{
            IfName: "eth0",
            AdvertiseInterval: 100,
        },
    }
    return
}

// genereate default hub config
func genHubConfig(fname string) (cfg *hubConfig) {
    cfg = new(hubConfig)
    cfg.Lan = genLanConfig()
    cfg.Sam = genDefaultSamConfig(false)
    cfg.ExitDest = "exit.psi.i2p"
    return
}

// generate default exit config
func genExitConfig(fname string) (cfg *exitConfig) {
    cfg = new(exitConfig)
    cfg.Exit = genDefaultExitNet()
    cfg.Exits = []exitNetConfig{}
    return
}

// load exit node config
func loadExitConfig(fname string) (cfg exitConfig, err error) {
    var data []byte
    data, err = ioutil.ReadFile(fname)
    if err == nil {
        err = json.Unmarshal(data, &cfg)
    }
    return
}


// load client hub config
func loadHubConfig(fname string) (cfg hubConfig, err error) {
    var data []byte
    data, err = ioutil.ReadFile(fname)
    if err == nil {
        err = json.Unmarshal(data, &cfg)
    }
    return
}

