
#ifndef _FACTORY_H_
#define _FACTORY_H_

#include "utils.h"
#include "common/object_pool.h"
#include <map>
#include <string>
#include <unordered_map>

#define BLOCKS      static_cast<int64_t>(65536)
#define UNUSED_SIZE static_cast<int64_t>(2048)
#define MAX_BUFSIZE static_cast<int64_t>(32 * 1024 * 1024)

enum eCrypto
{
	kAES,
	kBase64,
	kDES,
	kDES3,
	kBlowfish,
	kMax
};

inline bool is_vaild_coder(const eCrypto& code)
{
	return code >= eCrypto::kAES && code < eCrypto::kMax;
}

inline bool is_vaild_coder(const int& code)
{
	return code >= static_cast<int>(eCrypto::kAES) && code < static_cast<int>(eCrypto::kMax);
}

inline bool is_vaild_coder(const Slice& code)
{
	if (code.icompare("AES") == 0
		|| code.icompare("DES") == 0
		|| code.icompare("Base64") == 0
		|| code.icompare("Blowfish") == 0
	)
	{
		return true;
	}

	return false;
}

typedef std::pair<int, int> CryptoVersion;

const CryptoVersion v0 = CryptoVersion(0, 0);
const CryptoVersion v1 = CryptoVersion(1, 1);
const CryptoVersion v2 = CryptoVersion(2, 2);

extern std::map<int, std::string> FileNameCrypto;
extern std::map<int, std::string> FileDataCrypto;
extern void init_global();

struct CryptoMeta
{
	int      mFileNameVersion;
	int      mFileDataVersion;
	int      mFileNameLength;
	uint64_t mFileEnd;
	u_char   mMd5Sum[128];
	int      mMd5SumLength;

	bool is_vaild() const
	{
		return contains(FileDataCrypto, mFileDataVersion)
			&& contains(FileNameCrypto, mFileNameVersion)
			&& mFileNameLength >= 0;
	}
};

inline std::ostream& operator<<(std::ostream& out, const CryptoMeta& meta)
{
	out << "name : " << meta.mFileNameVersion << ", "
		<< "version : " << meta.mFileDataVersion << ", "
		<< "namelen : " << meta.mFileNameLength  << ", "
		<< "md5 : " << meta.mMd5Sum;
	return out;
}

struct FileMeta
{
	Slice mName;
	CryptoMeta  mMeta;
	uint64_t    mOffset;
	uint64_t    mFileSize;
	int         mWriteBSize;

	FileMeta()
	  : mOffset(0)
	  , mFileSize(0)
	  , mWriteBSize(0)
	{
		memset(&mMeta, 0, sizeof(mMeta));
	}
};

inline std::ostream& operator<<(std::ostream& out, const FileMeta& fmeta)
{
	out << "name  : "  << fmeta.mName     << ", "
		<< "meta  : {" << fmeta.mMeta     << "}, "
		<< "offset: "  << fmeta.mOffset   << ", "
		<< "fsize : "  << fmeta.mFileSize << ", "
		<< "wbsize: "  << fmeta.mWriteBSize;
	return out;
}

struct SliceHash 
{ 
    size_t operator()(const Slice& str) const
    {
    	size_t key = 0;
    	const char* last = str.cend();
    	for (const char* xpos = str.cbegin(); xpos != last; ++xpos)
    	{
    		key = key * 31ul + static_cast<uint8_t>(*xpos);
    	}
    	return key;
    }
};

typedef ObjectPool<std::string> StringPool;
typedef std::unordered_map<Slice, FileMeta, SliceHash> FileList;

void encode_filename(std::string& dest, const Slice& filename, const Slice& key, int version);
void decode_filename(std::string& dest, const Slice& filename, const Slice& key, int version);

Crypto* new_coder(const Slice& code, const Slice& key);
Crypto* new_coder2(const eCrypto& code, const Slice& key);

#endif // _FACTORY_H_
