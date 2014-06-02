#include "FileReaderWriter.h"

FileReaderWriter::FileReaderWriter( std::string fileName )
: m_File( nullptr ),
  m_LastErrorString( "" ),
  m_FileName( fileName )
{

}

FileReaderWriter::~FileReaderWriter()
{
    if( m_File )
    {
        fclose( m_File );
        m_File = nullptr;
    }
}

bool FileReaderWriter::OpenFile()
{
    bool retVal = true;

    m_File = fopen ( m_FileName.c_str(), "w+b");

    if( !m_File )
    {
        retVal = false;
        m_LastErrorString = std::string( strerror( errno ) );
    }

    return retVal;
}

bool FileReaderWriter::WriteBinaryData( const void* data, size_t sizeOfDataInBytes)
{
    bool retVal = false;

    if( m_File )
    {
        if( ( fwrite( data, 1, sizeOfDataInBytes, m_File ) == sizeOfDataInBytes ) &&
            !ferror( m_File ) )
        {
        	if(fflush(m_File) == 0)
        		retVal = true;
        }
        else
        {
            m_LastErrorString = std::string( strerror( errno ) );
        }
    }

    return retVal;
}

bool FileReaderWriter::ReadBinaryData( unsigned char* data, size_t sizeOfDataInBytes)
{
    bool retVal = false;

    if( m_File )
    {
        if( ( fread( data, 1, sizeOfDataInBytes, m_File ) == sizeOfDataInBytes ) &&
            !ferror( m_File ) )
        {
        		retVal = true;
        }
        else
        {
            m_LastErrorString = std::string( strerror( errno ) );
        }
    }

    return retVal;
}

fpos_t FileReaderWriter::GetCurrentSeek()
{
	fpos_t seek;
	int ret = fgetpos(m_File, &seek);

	if(ret != 0 )
		m_LastErrorString = std::string( strerror( errno ) );

	return seek;
}

void FileReaderWriter::SetCurrentSeek(fpos_t seek)
{
	fsetpos(m_File, &seek);
}

void FileReaderWriter::SeekToBegin()
{
	fseek(m_File,0,SEEK_SET);
}

size_t FileReaderWriter::BytesUptoCurrentSeek()
{
	return ftell(m_File);
}

size_t FileReaderWriter::GetCurrentFileSize()
{
	fpos_t currentPos = GetCurrentSeek();
	fseek(m_File,0,SEEK_END);

	size_t fileSize = BytesUptoCurrentSeek();
	SetCurrentSeek(currentPos);
	return fileSize;
}
