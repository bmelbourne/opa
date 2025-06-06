package jwa

import (
	"errors"
	"fmt"
	"strconv"
)

// KeyType represents the key type ("kty") that are supported
type KeyType string

var keyTypeAlg = map[string]struct{}{"EC": {}, "oct": {}, "RSA": {}}

// Supported values for KeyType
const (
	EC             KeyType = "EC"  // Elliptic Curve
	InvalidKeyType KeyType = ""    // Invalid KeyType
	OctetSeq       KeyType = "oct" // Octet sequence (used to represent symmetric keys)
	RSA            KeyType = "RSA" // RSA
)

// Accept is used when conversion from values given by
// outside sources (such as JSON payloads) is required
func (keyType *KeyType) Accept(value any) error {
	var tmp KeyType
	switch x := value.(type) {
	case string:
		tmp = KeyType(x)
	case KeyType:
		tmp = x
	default:
		return fmt.Errorf("invalid type for jwa.KeyType: %T", value)
	}
	_, ok := keyTypeAlg[tmp.String()]
	if !ok {
		return errors.New("unknown Key Type algorithm")
	}

	*keyType = tmp
	return nil
}

// String returns the string representation of a KeyType
func (keyType KeyType) String() string {
	return string(keyType)
}

// UnmarshalJSON unmarshals and checks data as KeyType Algorithm
func (keyType *KeyType) UnmarshalJSON(data []byte) error {
	var quote byte = '"'
	var quoted string
	if data[0] == quote {
		var err error
		quoted, err = strconv.Unquote(string(data))
		if err != nil {
			return fmt.Errorf("failed to process signature algorithm: %w", err)
		}
	} else {
		quoted = string(data)
	}
	_, ok := keyTypeAlg[quoted]
	if !ok {
		return errors.New("unknown signature algorithm")
	}
	*keyType = KeyType(quoted)
	return nil
}
