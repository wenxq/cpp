
#ifndef _DES_H_
#define _DES_H_

#include "interface.h"
#include "cryptopp/des.h"

using CryptoPP::DESEncryption;
using CryptoPP::DESDecryption;
using CryptoPP::DES;

class DESCrypto : public Crypto
{
public:
	DESCrypto(const Slice& key)
		: mKey(key.to_string())
	{
		mKey.resize(DES::DEFAULT_KEYLENGTH);
		mEncoder.SetKey((u_char *)mKey.data(), mKey.size());
		mDecoder.SetKey((u_char *)mKey.data(), mKey.size());
		memset(mXor, 0, sizeof(mXor));
	}

	virtual int type() const
	{
		return static_cast<int>(eCrypto::kDES);
	}

	virtual const char* name() const
	{
		return "DES";
	}

	// encode:
	virtual int encode_length(int len) const
	{
		return len;
	}

	virtual int encode_outsize() const
	{
		return DES::BLOCKSIZE;
	}

	virtual int encode_blocksize() const
	{
		return DES::BLOCKSIZE;
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
		return DES::BLOCKSIZE;
	}

	virtual int decode_blocksize() const
	{
		return DES::BLOCKSIZE;
	}

	virtual void decode(const u_char* input, u_char* output)
	{
		mDecoder.ProcessAndXorBlock(input, mXor, output);
	}

private:
	DESEncryption mEncoder;
	DESDecryption mDecoder;
	std::string   mKey;
	u_char        mXor[DES::BLOCKSIZE];
};

#endif // _DES_H_
