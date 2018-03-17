
#include "decode_file.h"
#include "factory.h"
#include "utils.h"
#include "cryptopp/md5.h"
#include "common/object_pool.h"

DecodeFile::DecodeFile(const Slice& input, const Slice& key)
  : mInput(input)
{
	SMART_ASSERT(!key.empty());
	std::string* str = &(mPool.new_object());
	key.copy_to(str);
	mKey = make_slice(*str);
	load();
}

bool DecodeFile::pop(const Slice& fsrc, const Slice& fdest)
{
	try
	{
		BOOST_AUTO(iter, mFileList.find(fsrc));
		if (iter == mFileList.end())
			return false;

		return pop_impl(iter->second, fdest);
	}
	catch (std::exception& ex)
	{
		std::cerr << "pop Catch Exception: " << ex.what() << std::endl;
		return false;
	}
}

bool DecodeFile::get(const Slice& fsrc, FileMeta& fmeta)
{
	try
	{
		BOOST_AUTO(iter, mFileList.find(fsrc));
		if (iter != mFileList.end())
		{
			fmeta = iter->second;
			return true;
		}

		return false;
	}
	catch (std::exception& ex)
	{
		std::cerr << "get Catch Exception: " << ex.what() << std::endl;
		return false;
	}
}

bool DecodeFile::load()
{
	try
	{
		mFileList.clear();
		return load_impl();
	}
	catch (std::exception& ex)
	{
		std::cerr << "load Catch Exception: " << ex.what() << std::endl;
		return false;
	}
}

bool DecodeFile::pop_impl(const FileMeta& fmeta, const Slice& fdest)
{
	mInput.clear();
	if (mInput.seekpos() != fmeta.mOffset)
	{
		mInput.seekg(fmeta.mOffset, mInput.beg());
	}

	CryptoMeta& meta  = const_cast<CryptoMeta &>(fmeta.mMeta);
	uint64_t filesize = fmeta.mFileSize;

	CryptoPP::Weak::MD5 md5;

	SMART_ASSERT(meta.is_vaild() && contains(FileDataCrypto, meta.mFileDataVersion));
	CryptoPipeline pipe(FileDataCrypto[meta.mFileDataVersion], mKey, new_coder);

	if (pipe.write_bsize() != fmeta.mWriteBSize)
		return false;

	int buffer_size = integer_cast<int32_t>(pipe.write_bsize() * BLOCKS);
	buffer_size = integer_cast<int32_t>((MAX_BUFSIZE / buffer_size) * buffer_size);
	std::vector<u_char> store_buffer;
	store_buffer.resize(buffer_size);
	u_char* buffer = &(store_buffer[0]);

	uint64_t offset = fmeta.mOffset;
	OutputFileStream output(fdest);
	while (true)
	{
		int nread = buffer_size;
		if (integer_cast<uint64_t>(nread) > meta.mFileEnd - offset)
			nread = integer_cast<int>(meta.mFileEnd - offset);

		int n = mInput.read(buffer, nread);
		if (n == 0) break;
		SMART_ASSERT(n == nread);

		offset += n;
		Slice result = pipe.decode(buffer, n);
		SMART_ASSERT(result.size() % integer_cast<int32_t>(pipe.read_bsize()) == 0);

		int out = result.size();
		if (integer_cast<uint64_t>(out) > filesize)
			out = integer_cast<int>(filesize);

		filesize -= out;
		output.write(result.data(), out);
		md5.Update((const byte *)result.data(), integer_cast<size_t>(out));
	}
	output.close();
	SMART_ASSERT(filesize == 0 && offset == meta.mFileEnd);

	byte m[16];
	md5.Final(m);
	std::string check_sum = detail::md5_string(m);
	if (meta.mMd5SumLength == 0)
	{
		if (!mInput.name().ends_with(make_slice(check_sum)))
			return false;

		SMART_ASSERT(check_sum.size() < sizeof(meta.mMd5Sum));
		meta.mMd5SumLength = integer_cast<int>(check_sum.size());
		memcpy(meta.mMd5Sum, check_sum.data(), check_sum.size());
		return true;
	}

	return meta.mMd5SumLength == check_sum.size()
		&& memcmp(meta.mMd5Sum, check_sum.data(), check_sum.size()) == 0;
}

bool DecodeFile::load_impl()
{
	mInput.seekg(0, mInput.beg());

	char size_buf[UNUSED_SIZE];
	while (true)
	{
		int n = mInput.read(size_buf, 32);
		if (n == 0) break;
		if (n != 32) return false;

		// header:
		Slice header = make_slice(size_buf, 32);
		FileMeta fmeta;
		const uint32_t namelen = integer_cast<uint32_t>(atoi(header.substr(0, 6)));
		fmeta.mFileSize   = atoi(header.substr(6, 20));
		fmeta.mWriteBSize = integer_cast<int32_t>(atoi(header.substr(26, 6)));
		if (fmeta.mFileSize == 0 || namelen >= 2048)
			return false;

		// filename:
		std::string namebuf;
		namebuf.resize(namelen);
		n = mInput.read((char *)namebuf.data(), namelen);
		if (n != namelen) return false;

		// unused:
		n = mInput.read(size_buf, UNUSED_SIZE);
		if (n != UNUSED_SIZE) return false;

		CryptoMeta& meta = fmeta.mMeta;
		memcpy(&meta, size_buf, sizeof(CryptoMeta));
		if (!meta.is_vaild()) return false;

		fmeta.mOffset = mInput.seekpos();

		std::string& filename = mPool.new_object();
		decode_filename(filename, make_slice(namebuf), mKey, meta.mFileNameVersion);
		fmeta.mName = make_slice(filename);
		if (meta.mFileNameLength != 0)
		{
			fmeta.mName = fmeta.mName.substr(0, meta.mFileNameLength);
		}

		if (!IsUTF8(fmeta.mName))
		{
			fmeta.mName = DecodeFileName(fmeta.mName, mBuffPool.new_object());
		}

		// skip data:
		if (meta.mFileEnd > 0)
		{
			mInput.seekg(integer_cast<std::streamoff>(meta.mFileEnd), mInput.beg());
		}
		else
		{
			mInput.seekg(0, mInput.end());
			meta.mFileEnd = integer_cast<uint64_t>(mInput.seekpos());
		}

		BOOST_AUTO(result, mFileList.insert(std::make_pair(fmeta.mName, fmeta)));
 		if (!result.second) return false;
	}

	return true;
}
