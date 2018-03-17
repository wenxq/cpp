
#include "factory.h"
#include "common/base64.h"
#include "base64.h"
#include "des.h"
#include "aes.h"
#include "blowfish.h"

std::map<int, std::string> FileNameCrypto;
std::map<int, std::string> FileDataCrypto;

void init_global()
{
	// file name:
	FileNameCrypto[v0.first]  = "BASE64";        // v0
	FileNameCrypto[v1.first]  = "BASE64 -> AES"; // v1
	FileNameCrypto[v2.first]  = "AES -> BASE64"; // v2

	// extend:(name)
	FileNameCrypto[v2.first + 1] = "DES -> BASE64"; // v3
	FileNameCrypto[v2.first + 2] = "BASE64 -> DES"; // v4
	FileNameCrypto[v2.first + 3] = "Blowfish -> BASE64"; // v5
	FileNameCrypto[v2.first + 4] = "BASE64 -> Blowfish"; // v6

	// file data:
	FileDataCrypto[v0.second] = "AES";           // v0
	FileDataCrypto[v1.second] = "AES";           // v1
	FileDataCrypto[v2.second] = "Blowfish";      // v2

	// extend:(data)
	FileDataCrypto[v2.second + 1] = "BASE64";    // v3
	FileDataCrypto[v2.second + 1] = "DES";       // v4
}

void encode_filename(std::string& dest, const Slice& filename, const Slice& key, int version)
{
	if (version == v0.first)
	{
		filename.copy_to(&dest);
		base64_encode(dest);
	}
	else
	{
		CryptoPipeline pipe(FileNameCrypto[version], key, new_coder);

		pipe.encode(filename)
			.copy_to(&dest);
	}
}

void decode_filename(std::string& dest, const Slice& filename, const Slice& key, int version)
{
	if (version == v0.first)
	{
		filename.copy_to(&dest);
		base64_decode(dest);
	}
	else
	{
		CryptoPipeline pipe(FileNameCrypto[version], key, new_coder);

		pipe.decode(filename)
			.copy_to(&dest);
	}
}

Crypto* new_coder(const Slice& code, const Slice& key)
{
	if (code.icompare("AES") == 0)
	{
		return new AESCrypto(key);
	}
	else if (code.icompare("DES") == 0)
	{
		return new DESCrypto(key);
	}
	else if (code.icompare("BASE64") == 0)
	{
		return new Base64Crypto();
	}
	else if (code.icompare("Blowfish") == 0)
	{
		return new BlowfishCrypto(key);
	}

	SMART_ASSERT(0).msg("code not support");
	return nullptr;
}

Crypto* new_coder2(const eCrypto& code, const Slice& key)
{
	if (code == eCrypto::kAES)
	{
		return new AESCrypto(key);
	}
	else if (code == eCrypto::kDES)
	{
		return new DESCrypto(key);
	}
	else if (code == eCrypto::kBase64)
	{
		return new Base64Crypto();
	}
	else if (code == eCrypto::kBlowfish)
	{
		return new BlowfishCrypto(key);
	}

	SMART_ASSERT(0).msg("code not support");
	return nullptr;
}
