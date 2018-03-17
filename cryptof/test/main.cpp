
#include "manager.h"
#include <iostream>
#include <fstream>

void output(const FileList& flist)
{
	for (BOOST_AUTO(iter, flist.begin()); iter != flist.end(); ++iter)
	{
		BOOST_AUTO(&fmeta, iter->second);
		std::vector<char> buffer;
		std::cout << "name  : "  << GetFileName(fmeta.mName, buffer) << "\n, "
					<< "meta  : {" << fmeta.mMeta     << "}, \n"
					<< "offset: "  << fmeta.mOffset   << ", \n"
					<< "fsize : "  << fmeta.mFileSize << ", \n"
					<< "wbsize: "  << fmeta.mWriteBSize << "\n"
					<< std::endl;
	}
}

void pop_all(DecodeFilePtr decoder)
{
	int i = 0;
	const FileList& flist = decoder->filelist();
	for (BOOST_AUTO(iter, flist.begin()); iter != flist.end(); ++iter)
	{
		BOOST_AUTO(&fmeta, iter->second);
		std::vector<char> buffer;
		Slice fileid = fmeta.mName;
		Slice filename = GetFileName(fileid, buffer);
		std::cout << "decode: \"" << filename << "\":\n" << std::endl;
		Slice outfile = fileid.substr(fileid.rfind(PATH_SEP) + 1);
		decoder->pop(fileid, outfile);
		std::cout << "---------------------------" << std::endl;
	}
}

inline Slice new_strp(const char* str)
{
	return make_slice(str, integer_cast<int>(strlen(str)));
} 

int main(int argc, char* argv[])
{
	if (argc < 2)
	{
		std::cout << argv[0] << " [files]" << std::endl;
		return 0;
	}

	Slice key = "a2b4c6d8e0,";
	Slice tarfile = "cryptf_test.output";

	CryptoManager manager;

	FileElement elem;
	for (int i = 1; i < argc; ++i)
	{
		elem.mFileName = new_strp(argv[i]);
		manager.push(tarfile, elem);
	}

	DecodeFilePtr decoder = manager.get_decoder(tarfile, key);
	output(decoder->filelist());
	pop_all(decoder);

	std::cout << "Entry any key to exit ..." << std::endl;
	std::cin.get();
	return 0;
}
