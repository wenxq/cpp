
#ifndef _ENCODE_FILE_H_
#define _ENCODE_FILE_H_

#include "factory.h"
#include "stream.h"
#include "common/slice.h"
#include "common/noncopyable.h"
#include <memory>

class EncodeFile : boost::noncopyable
{
public:
	EncodeFile(const Slice& output, const Slice& key = "a2b4c6d8e0,");

	CryptoVersion default_version() const
	{
		return mDefault;
	}

	void default_version(const CryptoVersion& v)
	{
		mDefault = v;
	}

	bool push(const Slice& fin, const CryptoVersion* version = nullptr);
	bool get(const Slice& fsrc, FileMeta& fmeta);

	const FileList& filelist() const { return mFileList; }

private:
	bool push_impl(const Slice& fin, const CryptoVersion& version);

private:
	OutputFileStream mOutput;
	Slice            mKey;
	CryptoVersion    mDefault;
	FileList         mFileList;
	StringPool       mPool;
};

typedef std::shared_ptr<EncodeFile> EncodeFilePtr;

#endif // _ENCODE_FILE_H_
