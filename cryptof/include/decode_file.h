
#ifndef _DECODE_FILE_H_
#define _DECODE_FILE_H_

#include "factory.h"
#include "stream.h"
#include "common/slice.h"
#include <memory>
#include <unordered_map>
#include <vector>

typedef ObjectPool<std::vector<char> > BufferPool;

class DecodeFile : boost::noncopyable
{
public:
	DecodeFile(const Slice& input, const Slice& key = "a2b4c6d8e0,");

	bool load();
	bool get(const Slice& fsrc, FileMeta& fmeta);
	bool pop(const Slice& fsrc, const Slice& fdest);

	const FileList& filelist() const { return mFileList; }

private:
	bool load_impl();
	bool pop_impl(const FileMeta& fmeta, const Slice& fdest);

private:
	InputFileStream mInput;
	Slice     mKey;
	FileList        mFileList;
	StringPool      mPool;
	BufferPool      mBuffPool;
};

typedef std::shared_ptr<DecodeFile> DecodeFilePtr;

#endif // _DECODE_FILE_H_
