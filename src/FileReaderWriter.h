#ifndef FILEWRITER_H_INCLUDED
#define FILEWRITER_H_INCLUDED

#include <ostream>
#include <sstream>
#include <cstdio>
#include <cstring>
#include <cerrno>

class FileReaderWriter
{
public:

	FileReaderWriter( std::string fileName );
    virtual ~FileReaderWriter();

    bool OpenFile();

    bool WriteBinaryData( const void* data, size_t sizeOfDataInBytes);
    bool ReadBinaryData( unsigned char* data, size_t sizeOfDataInBytes);
    void SeekToBegin();
    size_t BytesUptoCurrentSeek();
    size_t GetCurrentFileSize();

    fpos_t GetCurrentSeek();
    void SetCurrentSeek(fpos_t seek);

    inline std::string GetErrorStr() const;
    inline  bool ErrorHasOccured() const;

private:
    FileReaderWriter(const FileReaderWriter& other);              // Never to be public
    FileReaderWriter& operator= (const FileReaderWriter & other); // Never to be public

private:
    FILE* m_File;
    std::string m_LastErrorString;
    std::string m_FileName;
};

inline std::string FileReaderWriter::GetErrorStr() const
{
    return m_LastErrorString;
}

//-----------------------------------------------------------------------

inline bool FileReaderWriter::ErrorHasOccured() const
{
    return !m_LastErrorString.empty();
}

//-----------------------------------------------------------------------

#endif // FILEWRITER_H_INCLUDED
