
#include "encode_file.h"
#include "utils.h"
#include "cryptopp/md5.h"

EncodeFile::EncodeFile(const Slice& output, const Slice& key)
  : mOutput(output)
  , mDefault(v0)
{
	SMART_ASSERT(!output.empty() && !key.empty());
	std::string* str = &(mPool.new_object());
	key.copy_to(str);
	mKey = make_slice(*str);
}

bool EncodeFile::push(const Slice& fsrc, const CryptoVersion* version)
{
	try
	{
		if (version == nullptr)
		{
			version = &mDefault;
		}

		return push_impl(fsrc, *version);
	}
	catch (std::exception& ex)
	{
		std::cerr << "push Catch Exception: " << ex.what() << std::endl;
		return false;
	}
}

bool EncodeFile::get(const Slice& fsrc, FileMeta& fmeta)
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
		std::cerr << "get_meta Catch Exception: " << ex.what() << std::endl;
		return false;
	}
}

bool EncodeFile::push_impl(const Slice& fsrc, const CryptoVersion& version)
{
	CryptoPipeline  pipe(FileDataCrypto[version.second], mKey, new_coder);
	InputFileStream input(fsrc);

	FileMeta fmeta;

	fmeta.mOffset = mOutput.seekpos();

	input.seekg(0, input.end());
	fmeta.mFileSize = integer_cast<uint64_t>(input.seekpos());
	input.seekg(0, input.beg());

	char size_fmt[UNUSED_SIZE];

	// filename.1:
	std::string& filename = mPool.new_object();
	encode_filename(filename, fsrc, mKey, version.first);
	fmeta.mName = make_slice(filename);
	SMART_ASSERT(fmeta.mName.size() < 999999);
	int count = snprintf(size_fmt, 12, "%06d", integer_cast<uint32_t>(fmeta.mName.size()));
	mOutput.write(size_fmt, count);

	// filesize:
	count = snprintf(size_fmt, 21, "%020lld", fmeta.mFileSize);
	mOutput.write(size_fmt, count);

	// write blocksize:
	fmeta.mWriteBSize = pipe.write_bsize();
	count = snprintf(size_fmt, 12, "%06d", fmeta.mWriteBSize);
	mOutput.write(size_fmt, count);

    // filename.2:
	mOutput.write(fmeta.mName.data(), fmeta.mName.size());

	// unused: <- meta
	SMART_ASSERT(sizeof(CryptoMeta) <= UNUSED_SIZE);
	const std::streampos meta_offset = mOutput.tellp();
	memset(size_fmt, 0, UNUSED_SIZE);
	mOutput.write(size_fmt, UNUSED_SIZE);

	int buffer_size = integer_cast<int32_t>(pipe.read_bsize() * BLOCKS);
	buffer_size = integer_cast<int32_t>((MAX_BUFSIZE / buffer_size) * buffer_size);
	const int out = integer_cast<int32_t>(pipe.write_bsize() * (buffer_size / pipe.read_bsize()));
	std::vector<u_char> store_buffer;
	store_buffer.resize(buffer_size);
	u_char* buffer = &(store_buffer[0]);

	int n = 0;
	CryptoPP::Weak::MD5 md5;
	while ((n = input.read((char *)buffer, buffer_size)) != 0)
	{
		md5.Update(buffer, integer_cast<size_t>(n));

		Slice result = pipe.encode(buffer, n);
		SMART_ASSERT(n < buffer_size || result.size() == out);
		mOutput.write(result.data(), result.size());
	}
	input.close();

	const std::streampos end_offset = mOutput.tellp();

	byte m[16];
	md5.Final(m);
	std::string check_sum = detail::md5_string(m);
	std::cout << end_offset << ", md5: " << check_sum << std::endl;

	CryptoMeta& meta = fmeta.mMeta;
	SMART_ASSERT(check_sum.size() < sizeof(meta.mMd5Sum));
	meta.mFileNameVersion = version.first;
	meta.mFileDataVersion = version.second;
	meta.mFileNameLength  = fsrc.size();
	meta.mMd5SumLength    = integer_cast<int>(check_sum.size());
	meta.mFileEnd         = integer_cast<uint64_t>(static_cast<std::streamoff>(end_offset));
	memcpy(meta.mMd5Sum, check_sum.data(), check_sum.size());

	mOutput.seekp(meta_offset);
	mOutput.write(&meta, sizeof(meta));
	mOutput.seekp(end_offset);
	mOutput.flush();

	BOOST_AUTO(result, mFileList.insert(std::make_pair(fmeta.mName, fmeta)));
	return result.second;
}
