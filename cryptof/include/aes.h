
#include "interface.h"
#include "cryptopp/aes.h"

using CryptoPP::AESEncryption;
using CryptoPP::AESDecryption;
using CryptoPP::AES;

class AESCrypto : public Crypto
{
public:
	AESCrypto(const Slice& key)
	  : mKey(key.to_string())
	{
		mKey.resize(AES::DEFAULT_KEYLENGTH);
		mEncoder.SetKey((u_char *)mKey.data(), mKey.size());
		mDecoder.SetKey((u_char *)mKey.data(), mKey.size());
		memset(mXor, 0, sizeof(mXor));
	}

	virtual int type() const
	{
		return static_cast<int>(eCrypto::kAES);
	}

	virtual const char* name() const
	{
		return "AES";
	}

	// encode:
	virtual int encode_length(int len) const
	{
		return len;
	}

	virtual int encode_outsize() const
	{
		return AES::BLOCKSIZE;
	}

	virtual int encode_blocksize() const
	{
		return AES::BLOCKSIZE;
	}

	virtual void encode(const u_char* input, u_char* output)
	{
		mEncoder.ProcessAndXorBlock(input, mXor, output);
	}

	// decode:
	virtual int decode_length(int len) const
	{
		return len;
	}

	virtual int decode_outsize() const
	{
		return AES::BLOCKSIZE;
	}

	virtual int decode_blocksize() const
	{
		return AES::BLOCKSIZE;
	}

	virtual void decode(const u_char* input, u_char* output)
	{
		mDecoder.ProcessAndXorBlock(input, mXor, output);
	}

private:
	AESEncryption mEncoder;
	AESDecryption mDecoder;
	std::string   mKey;
	u_char        mXor[AES::BLOCKSIZE];
};

