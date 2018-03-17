
#ifndef _MANAGER_H_
#define _MANAGER_H_

#include "encode_file.h"
#include "decode_file.h"
#include "common/object_pool.h"
#include <iostream>

struct FileElement
{
	CryptoVersion mVersion;
	Slice   mKey;
	Slice   mFileName;
	Slice   mOutput;

	FileElement()
	  : mVersion(v0)
	  , mKey("a2b4c6d8e0,")
	{}
};

class CryptoManager : boost::noncopyable
{
public:
	CryptoManager()
	  : mFiles(8)
	{
		init_global();
	}

	~CryptoManager()
	{
	}

	bool push(const Slice& tarfile, const FileElement& elem)
	{
		BOOST_AUTO(iter, mEncoder.find(tarfile));
		if (iter == mEncoder.end())
		{
			std::string& str = mFiles.new_object();
			tarfile.copy_to(&str);
			Slice output = make_slice(str);

			EncodeFilePtr encoder(new EncodeFile(output, elem.mKey));
			encoder->default_version(elem.mVersion);

			BOOST_AUTO(result, mEncoder.insert(std::make_pair(output, encoder)));
			if (!result.second) return false;
			iter = result.first;
		}

		const CryptoVersion default_version = iter->second->default_version();
		const CryptoVersion* version = &default_version;
		if (elem.mVersion != *version)
		{
			version = &(elem.mVersion);
		}

		if (!iter->second->push(elem.mFileName, version))
			return false;

		std::string filename;
		base64_encode(elem.mFileName, filename);

		std::cout << "uuid: \"" << tarfile
			<< "\", filename: \"" << make_slice(filename)
			<< "\", desc: " << 1
			<< std::endl;
		
		return true;
	}

	bool pop(const Slice& tarfile, const FileElement& elem)
	{
		BOOST_AUTO(iter, mDecoder.find(tarfile));
		if (iter == mDecoder.end())
		{
			std::string& str = mFiles.new_object();
			tarfile.copy_to(&str);
			Slice output = make_slice(str);

			DecodeFilePtr decoder(new DecodeFile(output, elem.mKey));
			BOOST_AUTO(result, mDecoder.insert(std::make_pair(output, decoder)));
			if (!result.second) return false;
			iter = result.first;
		}

		return iter->second->pop(elem.mFileName, elem.mOutput);
	}

	EncodeFilePtr get_encoder(const Slice& tarfile)
	{
		BOOST_AUTO(iter, mEncoder.find(tarfile));
		if (iter == mEncoder.end())
			return EncodeFilePtr();

		return iter->second;
	}

	DecodeFilePtr get_decoder(const Slice& tarfile, const Slice& key)
	{
		BOOST_AUTO(iter, mDecoder.find(tarfile));
		if (iter == mDecoder.end())
		{
			std::string& str = mFiles.new_object();
			tarfile.copy_to(&str);
			Slice infile = make_slice(str);

			DecodeFilePtr decoder(new DecodeFile(infile, key));
			BOOST_AUTO(result, mDecoder.insert(std::make_pair(infile, decoder)));
			if (!result.second) return DecodeFilePtr();
			iter = result.first;
		}

		return iter->second;
	}

private:
	typedef std::unordered_map<Slice, EncodeFilePtr, SliceHash> EncoderList;
	typedef std::unordered_map<Slice, DecodeFilePtr, SliceHash> DecoderList;

	ObjectPool<std::string> mFiles;
	EncoderList mEncoder;
	DecoderList mDecoder;
};

#endif // _MANAGER_H_
