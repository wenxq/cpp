
#ifndef _STREAM_H_
#define _STREAM_H_

#include "exception.h"
#include "utils.h"
#include "common/slice.h"
#include <fstream>

class InputFileStream : boost::noncopyable
{
public:
	InputFileStream(const Slice& filename)
	  : mFileName(GetFileName(filename, mBuffer))
	{
		mIn.open(mFileName.data(), std::ios::binary | std::ios::in);
		if (!mIn.good())
		{
			std::cerr << "\ninput: " << mFileName.data() << std::endl;
			throw Exception("open input file failed");
		}
	}

	~InputFileStream()
	{
		close();
	}

	Slice name() const
	{
		return mFileName;
	}

	void close()
	{
		if (mIn.is_open())
		{
			mIn.close();
		}
	}

	void clear()
	{
		mIn.clear();
	}

	std::ios_base::seekdir beg() const { return mIn.beg; }
	std::ios_base::seekdir end() const { return mIn.end; }
	std::ios_base::seekdir cur() const { return mIn.cur; }

	InputFileStream& seekg(const std::streampos& pos)
	{
		mIn.seekg(pos);
		if (mIn.fail())
		{
			throw Exception("seekg file failed");
		}

		return *this;
	}

	InputFileStream& seekg(const std::streamoff& off, std::ios_base::seekdir way)
	{
		mIn.seekg(off, way);
		if (mIn.fail())
		{
			throw Exception("seekg file failed");
		}

		return *this;
	}

	std::streampos tellg()
	{
		return mIn.tellg();
	}

	std::streamoff seekpos()
	{
		return mIn.tellg();
	}

	int read(void* buffer, int size)
	{
		if (mIn.eof()) return 0;

		if (mIn.fail())
		{
			throw Exception("read data from file failed");
		}

		mIn.read((char *)buffer, integer_cast<std::streamsize>(size));
		if (mIn.fail() && !mIn.eof())
		{
			throw Exception("read data from file failed");
		}

		return integer_cast<int>(mIn.gcount());
	}

private:
	std::ifstream mIn;
	Slice   mFileName;
	std::vector<char> mBuffer;
};

class OutputFileStream : boost::noncopyable
{
public:
	OutputFileStream(const Slice& filename)
	  : mFileName(GetFileName(filename, mBuffer))
	{
		mOut.open(mFileName.data(), std::ios::binary | std::ios::out | std::ios::trunc);
		if (mOut.fail())
		{
			std::cerr << "\noutput: " << mFileName << std::endl;
			throw Exception("open output file failed");
		}
	}

	~OutputFileStream()
	{
		close();
	}

	Slice name() const
	{
		return mFileName;
	}

	void close()
	{
		if (mOut.is_open())
		{
			mOut.close();
		}
	}

	void clear()
	{
		mOut.clear();
	}

	std::ios_base::seekdir beg() const { return mOut.beg; }
	std::ios_base::seekdir end() const { return mOut.end; }
	std::ios_base::seekdir cur() const { return mOut.cur; }

	OutputFileStream& seekp(const std::streampos& pos)
	{
		mOut.seekp(pos);
		if (mOut.fail())
		{
			throw Exception("seekp file failed");
		}

		return *this;
	}

	OutputFileStream& seekp(const std::streamoff& off, std::ios_base::seekdir way)
	{
		mOut.seekp(off, way);
		if (mOut.fail())
		{
			throw Exception("seekp file failed");
		}

		return *this;
	}

	std::streampos tellp()
	{
		return mOut.tellp();
	}

	std::streamoff seekpos()
	{
		return mOut.tellp();
	}

	int write(const void* buffer, int size)
	{
		if (mOut.fail())
		{
			throw Exception("write data to file failed");
		}

		mOut.write((const char *)buffer, integer_cast<std::streamsize>(size));
		if (mOut.fail())
		{
			throw Exception("write data to file failed");
		}

		return size;
	}

	OutputFileStream& flush()
	{
		mOut.flush();
		if (mOut.fail())
		{
			throw Exception("flush file data failed");
		}

		return *this;
	}

private:
	std::ofstream mOut;
	Slice   mFileName;
	std::vector<char> mBuffer;
};

#endif // _STREAM_H_
