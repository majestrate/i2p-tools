package samtun
//
// bencoding implementation for samtun
//

import (
    "errors"
    "fmt"
    "io"
    "sort"
    "strconv"
)

func BencodeWriteString(str []byte, w io.Writer) (err error) {
    // length
    _, err = io.WriteString(w, strconv.Itoa(len(str)))
    if err != nil {
        return
    }
    // ':'
    _, err = w.Write([]byte{58})
    if err != nil {
        return
    }
    // string
    err = writeAll(str, w)
    return
}

func BencodeWriteMap(m map[string]interface{}, w io.Writer) (err error) {
    // 'd'
    _, err = w.Write([]byte{100})
    // get keys in lex order
    var keys []string
    for k, _ := range m {
        keys = append(keys, k)
    }
    sort.Strings(keys)
    // write entries
    for _, k := range keys {
        v := m[k]
        err = BencodeWriteString([]byte(k), w)
        if err != nil {
            return
        }
        err = BencodeWrite(v, w)
        if err != nil {
            return
        }
    }
    // 'e'
    _, err = w.Write([]byte{101})
    return
}

func BencodeWriteList(ls []interface{}, w io.Writer) (err error) {
    // 'l'
    _, err = w.Write([]byte{108})
    if err != nil {
        return
    }
    for _, item := range ls {
        err = BencodeWrite(item, w)
        if err != nil {
            return
        }
    }
    // 'e'
    _, err = w.Write([]byte{101})
    return
}

func BencodeWriteInt(i int64, w io.Writer) (err error) {
    // 'i'
    _, err = w.Write([]byte{105})
    if err != nil {
        return
    }
    // number base 10
    _, err = io.WriteString(w, strconv.FormatInt(i, 10))
    if err != nil {
        return
    }
    // 'e'
    _, err = w.Write([]byte{101})
    return
}

func BencodeWrite(obj interface{}, w io.Writer) (err error) {
    switch obj.(type) {
    case int64:
        err = BencodeWriteInt(obj.(int64), w)
    case string:
        err = BencodeWriteString([]byte(obj.(string)), w)
    case []byte:
        err = BencodeWriteString(obj.([]byte), w)
    case map[string]interface{}:
    case map[string]string:
    case map[string]int64:
        err = BencodeWriteMap(obj.(map[string]interface{}), w)
    case []interface{}:
    case []int64:
    case [][]byte:
    case []string:
        err = BencodeWriteList(obj.([]interface{}), w)
    default:
        err = errors.New("cannot bencode write "+fmt.Sprintf("%q", obj))
    }
    return
}

func bencode_readString(r io.Reader, prev byte) (str []byte, err error) {
    var lenbuff []byte
    if prev > 0 {
        // we have a previous byte
        lenbuff = append(lenbuff, prev)
    }
    var b [1]byte
    var c byte
    n := 0
    for {
        _, err = r.Read(b[:])
        if err != nil {
            return
        }
        c = b[0]
        if c > 47 && c < 58 {
            // number
            if n == 0 && c == 48 {
                // cannot start with '0'
                err = errors.New("invalid bytestring length starting with '0'")
                return
            } else {
                lenbuff = append(lenbuff, c)
            }
        } else if c == 58 {
            // ':'
            i, _ := strconv.Atoi(string(lenbuff))
            str, err = readExactly(i, r)
            return
        } else {
            // invalid
            err = errors.New("invalid char in bytestring length: "+string(c))
            return
        }
    }
}

func bencode_readMap(r io.Reader, prev byte) (m map[string]interface{}, err error) {
    var b [1]byte
    var c byte
    var kb []byte
    if prev == 0 {
        _, err = r.Read(b[:])
        c = b[0]
        if c != 100 {
            // invalid char
            err = errors.New("not a bencode dict")
        }
    }
    m = make(map[string]interface{})
    for err == nil {
        // read key
        kb, err = bencode_readString(r, 0)
        if err == nil {
            k := string(kb)
            _, err = r.Read(b[:])
            c = b[0]
            if err != nil {
                // error
                return
            } else if c == 101 {
                // we dun
                return
            } else if c > 48 && c < 58 {
                // is string
                var s []byte
                s, err = bencode_readString(r, c)
                if err == nil {
                    // gud
                    m[k] = s
                    continue
                }
            } else if c == 100 {
                // another dict
                var d map[string]interface{}
                d, err = bencode_readMap(r, c)
                if err == nil {
                    // gud
                    m[k] = d
                    continue
                }
            } else if c == 108 {
                // a list
                var l []interface{}
                l, err = bencode_readList(r, c)
                if err == nil {
                    // gud
                    m[k] = l
                    continue
                }
            } else if c == 105 {
                // an int
                var i int64
                i, err = bencode_readInt(r, c)
                if err == nil {
                    // gud
                    m[k] = i
                    continue
                }
            }
        }
    }
    return
}

func bencode_readList(r io.Reader, prev byte) (ls []interface{}, err error) {
    var b [1]byte
    var c byte
    var str []byte
    var d map[string]interface{}
    var l []interface{}
    var i int64

    _, err = r.Read(b[:])
    c = b[0]
    if prev == 108 && c == 101 {
        ls = []interface{}{} // empty list
        return
    } else if prev == 0 && c != 108 {
        // we expected a list but got something else?
        err = errors.New("Not a list, got byte: "+string(c))
    } else {
        for err == nil {
            if c == 101 {
                // terminate list
                break
            }
            if c > 48 && c < 58 {
                // this a string
                str, err = bencode_readString(r, c)
                if err == nil {
                    ls = append(ls, str)
                }
            } else if c == 100 {
                // dict
                d, err = bencode_readMap(r, c)
                if err == nil {
                    ls = append(ls, d)
                }
            } else if c == 108 {
                // another list
                l, err = bencode_readList(r, c)
                if err == nil {
                    ls = append(ls, l)
                }
            } else if c == 105 {
                // int
                i, err = bencode_readInt(r, c)
                if err == nil {
                    ls = append(ls, i)
                }
            }
            _, err = r.Read(b[:])
            c = b[0]
        }
    }
    return
}

func bencode_readInt(r io.Reader, prev byte) (i int64, err error) {
    var intbuff []byte
    var b [1]byte
    var c byte
    n := 0
    if prev == 0 {
        _, err = r.Read(b[:])
        if err != nil {
            return
        }
        prev = b[0]
    }
    if prev == 105 { // 'i'
        // read until 'e'
        for {
            _, err = r.Read(b[:])
            if err != nil {
                return
            }
            c = b[0]
            if c == 101 {
                // we got 'e'
                i, err = strconv.ParseInt(string(intbuff), 10, 64)
                return
            } else if c > 47 && c < 58 {
                if n == 0 && c == 48 {
                    err = errors.New("base 10 numbers can't start with '0'")
                    return
                } else {
                    intbuff = append(intbuff, c)
                    n  ++
                } 
            } else {
                // invalid char
                err = errors.New("invalid char in int: "+ string(c))
                return
            }
        }
    } else { // not 'i'
        err = errors.New("expected 'i' got '"+ string(prev)+ "'")
    }
    return
}

func BencodeReadString(r io.Reader) ([]byte, error) {
    return bencode_readString(r, 0)
}

func BencodeReadMap(r io.Reader) (map[string]interface{}, error) {
    return bencode_readMap(r, 0)
}

func BencodeReadList(r io.Reader) ([]interface{}, error) {
    return bencode_readList(r, 0)
}
