test_run = require('test_run').new()
test_run:cmd("push filter ".."'\\.lua.*:[0-9]+: ' to '.lua:<line>\"]: '")

crypto = require('crypto')
type(crypto)

ciph = crypto.cipher.aes128.cbc
pass = '1234567887654321'
iv = 'abcdefghijklmnop'
enc = ciph.encrypt('test', pass, iv)
enc
ciph.decrypt(enc, pass, iv)


--Failing scenaries
crypto.cipher.aes128.cbc.encrypt('a')
crypto.cipher.aes128.cbc.encrypt('a', '123456', '435')
crypto.cipher.aes128.cbc.encrypt('a', '1234567887654321')
crypto.cipher.aes128.cbc.encrypt('a', '1234567887654321', '12')

crypto.cipher.aes256.cbc.decrypt('a')
crypto.cipher.aes256.cbc.decrypt('a', '123456', '435')
crypto.cipher.aes256.cbc.decrypt('a', '12345678876543211234567887654321')
crypto.cipher.aes256.cbc.decrypt('12', '12345678876543211234567887654321', '12')

crypto.cipher.aes192.cbc.encrypt.new('123321')
crypto.cipher.aes192.cbc.decrypt.new('123456788765432112345678', '12345')

--
-- It is allowed to fill a cipher gradually.
--
c = crypto.cipher.aes128.cbc.encrypt.new()
c:init('1234567812345678')
c:update('plain')
c:init(nil, '1234567812345678')
c:update('plain')
c:free()

--
-- gh-4223: cipher:init() could rewrite previously initialized
-- key and IV with nil values.
--
c = crypto.cipher.aes192.cbc.encrypt.new('123456789012345678901234', '1234567890123456')
c:init()
c:result()
c:free()

crypto.cipher.aes100.efb
crypto.cipher.aes256.nomode

crypto.digest.nodigest


bad_pass = '8765432112345678'
bad_iv = '123456abcdefghij'
ciph.decrypt(enc, bad_pass, iv)
ciph.decrypt(enc, pass, bad_iv)

test_run:cmd("clear filter")
