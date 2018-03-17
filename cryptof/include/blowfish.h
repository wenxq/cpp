
#include "interface.h"
#include "cryptopp/blowfish.h"

using CryptoPP::BlowfishEncryption;
using CryptoPP::BlowfishDecryption;
using CryptoPP::Blowfish;

class BlowfishCrypto : public Crypto
{
public:
	BlowfishCrypto(const Slice& key)
		: mKey(key.to_string())
	{
		mKey.resize(Blowfish::DEFAULT_KEYLENGTH);
		mEncoder.SetKey((u_char *)mKey.data(), mKey.size());
		mDecoder.SetKey((u_char *)mKey.data(), mKey.size());
		memset(mXor, 0, sizeof(mXor));
	}

	virtual int type() const
	{
		return static_cast<int>(eCrypto::kBlowfish);
	}

	virtual const char* name() const
	{
		return "Blowfish";
	}

	// encode:
	virtual int encode_length(int len) const
	{
		return len;
	}

	virtual int encode_outsize() const
	{
		return Blowfish::BLOCKSIZE;
	}

	virtual int encode_blocksize() const
	{
		return Blowfish::BLOCKSIZE;
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
		return Blowfish::BLOCKSIZE;
	}

	virtual int decode_blocksize() const
	{
		return Blowfish::BLOCKSIZE;
	}

	virtual void decode(const u_char* input, u_char* output)
	{
		mDecoder.ProcessAndXorBlock(input, mXor, output);
	}

private:
	BlowfishEncryption mEncoder;
	BlowfishDecryption mDecoder;
	std::string        mKey;
	u_char             mXor[Blowfish::BLOCKSIZE];
};
