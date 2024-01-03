/***************************************************************************
*
*  SRecMem.cpp
*
*  This program reads an S-Record file and put it into memory
*  which contains the data.
*
****************************************************************************/

/* ---- Include Files ---------------------------------------------------- */

#include <string.h>
#include "SRecMem.h"

/* ---- Public Variables ------------------------------------------------- */
/* ---- Private Constants and Types -------------------------------------- */

/* ---- Private Variables ------------------------------------------------ */
/* ---- Private Function Prototypes -------------------------------------- */

//**************************************************************************

//**************************************************************************

SRecordMem::SRecordMem( void )
{
   memset(&m_header, 0, sizeof(m_header));
   m_startAddr = 0;
   m_segIdx = 0;
}

//**************************************************************************

SRecordMem::~SRecordMem( void )
{
    unsigned int i;
    for (i=0; i<m_memBlock.size(); i++)
    {
        m_memBlock[i].m_data.clear();
    }
    m_memBlock.clear();
}

void SRecordMem::PrintDebugInfo(bool en_print_byte)
{
   unsigned int i, j;
   std::cout << std::setfill( '0' );
   std::cout << std::setbase( 16 );
   std::cout << "Module: '" << m_header.m_module  << "', ";
   std::cout << "Ver: '"    << m_header.m_ver     << "', ";
   std::cout << "Rev: '"    << m_header.m_rev     << "', ";
   std::cout << "Description: '"  << m_header.m_comment << "'" << std::endl;
   std::cout << "Start Address: 0x" << std::setw( 8 ) << m_startAddr << std::endl;
   std::cout << "Segment Number: " << m_segIdx << std::endl << std::endl;
   for (i = 0; i < m_segIdx; i++)
   {
      std::cout << "Segment #" << i << std::endl
      << "Segment Load Address: 0x" << std::setw( 8 ) << m_memBlock[i].m_loadAddr << std::endl
      << "Segment Data Size:    0x" << std::setw( 8 ) << m_memBlock[i].m_dataLen << " bytes" << std::endl;
      if (en_print_byte)
      {
         std::cout << "Data (Hex):";
         for (j=0; j < m_memBlock[i].m_dataLen; j++)
         {
            if (j % 16 == 0)
            {
               std::cout << std::endl;
            }
            std::cout << std::setw( 2 ) << (unsigned)m_memBlock[i].m_data[j] << " ";
         }
         std::cout << std::endl << std::endl;
      }
    }
}

unsigned int SRecordMem::GetSegmentNumber( void )
{
    return m_segIdx;
}

bool SRecordMem::GetSegmentInfo(unsigned int seg_index, unsigned int *start_address, unsigned int *data_len)
{
    bool ret;
    if (seg_index + 1 > m_segIdx)
    {
        ret = false;
    }
    else
    {
        *start_address = m_memBlock[seg_index].m_loadAddr;
        *data_len = m_memBlock[seg_index].m_dataLen;
        ret = true;
    }
    return ret;
}

unsigned int SRecordMem::ReadSegmentData (unsigned int seg_index, unsigned int byte_offset, unsigned char *buff, unsigned int buf_size)
{
    unsigned int len;
    unsigned int tmp;

    if (seg_index + 1 > m_segIdx)
    {
        len = 0;
    }
    else if (byte_offset + 1 > m_memBlock[seg_index].m_dataLen)
    {
        len = 0;
    }
    else
    {
        tmp = m_memBlock[seg_index].m_dataLen - byte_offset;
        len = (buf_size > tmp) ? tmp : buf_size;
        memcpy(buff, &m_memBlock[seg_index].m_data[0], len);
    }
    return len;
}

unsigned int SRecordMem::GetData(unsigned int address, unsigned int size, unsigned char *buff, unsigned char pad_byte)
{
	unsigned int ret = 0;
	unsigned int i;
	unsigned int len;
	unsigned int offset;
	memset(buff, pad_byte, size);
	for (i=0; i<m_segIdx; i++)
	{
		if ((m_memBlock[i].m_loadAddr <= address) && (m_memBlock[i].m_loadAddr + m_memBlock[i].m_dataLen > address))
		{
			offset = address - m_memBlock[i].m_loadAddr;
			len = (size < (m_memBlock[i].m_dataLen - offset)) ? size : (m_memBlock[i].m_dataLen - offset);
			memcpy(buff, &m_memBlock[i].m_data[offset], len);
			buff += len;
			ret += len;
			size -= len;
			address += len;
		}
	}
	return ret;
}

unsigned char* SRecordMem::GetSegmentDataPointer (unsigned int seg_index, unsigned int *data_len)
{
    unsigned char *ret;
    if (seg_index + 1 > m_segIdx)
    {
        ret = (unsigned char *)NULL;
        *data_len = 0;
    }
    else
    {
        ret = &m_memBlock[seg_index].m_data[0];
        *data_len = m_memBlock[seg_index].m_dataLen;
    }
    return ret;
}
void SRecordMem::GetModuleName(char *module_name)
{
    strncpy(module_name, &m_header.m_module[0], 21);
}

void SRecordMem::SetModuleName(char *module_name)
{
    memset(m_header.m_module, 0, sizeof(m_header.m_module));
    strncpy(m_header.m_module, module_name, 21);
}
void SRecordMem::GetVersion(char *ver)
{
    strncpy(ver, &m_header.m_ver[0], 3);
}
void SRecordMem::SetVersion(char *ver)
{
    memset(m_header.m_ver, 0, sizeof(m_header.m_ver));
    strncpy(m_header.m_ver, ver, 3);
}
void SRecordMem::GetRevision(char *rev)
{
    strncpy(rev, &m_header.m_rev[0], 3);
}
void SRecordMem::SetRevision(char *rev)
{
    memset(m_header.m_rev, 0, sizeof(m_header.m_rev));
    strncpy(m_header.m_rev, rev, 3);
}
void SRecordMem::GetComment(char *comment)
{
    strncpy(comment, &m_header.m_comment[0], 37);
}
void SRecordMem::SetComment(char *comment)
{
    memset(m_header.m_comment, 0, sizeof(m_header.m_comment));
    strncpy(m_header.m_comment, comment, 37);
}

//**************************************************************************


//**************************************************************************
// virtual

bool SRecordMem::Data( const SRecordData *sRecData )
{
   int i;
   for (i=0; i<sRecData->m_dataLen; i++)
   {
      m_memBlock[m_segIdx].m_data.push_back( sRecData->m_data[i] );
   }
   return true;
}

//**************************************************************************
// virtual

bool SRecordMem::FinishSegment( unsigned addr, unsigned len )
{
   bool ret;
   if (m_memBlock.size() == m_segIdx +1)
   {
       if (m_memBlock[m_segIdx].m_loadAddr == addr)
       {
           if (m_memBlock[m_segIdx].m_data.size() == len)
           {
              m_memBlock[m_segIdx].m_dataLen = len;
              m_segIdx++;
              ret = true;
           }
           else
           {
               ret = false;
           }

       }
       else
       {
           ret = false;
       }
   }
   else
   {
       ret = false;
   }
   return ret;
}

//**************************************************************************
// virtual

bool SRecordMem::Header( const SRecordHeader *sRecHdr )
{
   memcpy (&m_header, sRecHdr, sizeof(m_header));
   return true;
}


//**************************************************************************
// virtual

bool SRecordMem::StartAddress( const SRecordData *sRecData )
{
   m_startAddr = sRecData->m_addr;

   return true;
}

//**************************************************************************
// virtual

bool SRecordMem::StartSegment( unsigned addr )
{
   SRecMemBlock  sRecMemBlock;
   sRecMemBlock.m_loadAddr = addr;
   sRecMemBlock.m_dataLen  = 0;
   m_memBlock.push_back( sRecMemBlock );
   return true;
}

void SRecordMem::WriteSrec(FILE *fs, int type, unsigned long address, unsigned char *data, int len)
{
    char tmp_buf[16];
    int i;
    int cs = 0;
    if (fs != NULL)
    {
        if (0 == type) //header record
        {
            address = 0;
            snprintf(tmp_buf,16, "S%d%02X%04X",type, len+3,(unsigned int)address);
            cs = len+3;
            cs += (unsigned char)(address >> 24);
            cs += (unsigned char)(address >> 16);
            cs += (unsigned char)(address >> 8);
            cs += (unsigned char)(address);
            fputs(tmp_buf, fs);
            for (i=0; i<len; i++)
            {
                cs += data[i];
                fprintf(fs,"%02X", data[i]);
            }
        }
        else if ((type > 0) && (type < 4)) //data record
        {
            if ((address >=0) && (address <= 0xFFFF))
            {
                type = 1;
                snprintf(tmp_buf, 16, "S%d%02X%04X",type, len+3,(unsigned int)address);
                cs = len + 3;
            }
            else if ((address > 0xFFFF) && (address <= 0xFFFFFF))
            {
                type = 2;
                snprintf(tmp_buf,16, "S%d%02X%06X",type, len+4,(unsigned int)address);
                cs = len + 4;
            }
            else
            {
                type = 3;
                snprintf(tmp_buf,16, "S%d%02X%08X",type, len+5,(unsigned int)address);
                cs = len + 5;
            }
            cs += (unsigned char)(address >> 24);
            cs += (unsigned char)(address >> 16);
            cs += (unsigned char)(address >> 8);
            cs += (unsigned char)(address);
            fputs(tmp_buf, fs);
            for (i=0; i<len; i++)
            {
                cs += data[i];
                fprintf(fs,"%02X", data[i]);
            }
        }
		else if (type == 5)
		{
			address = address & 0xFFFF;
			snprintf(tmp_buf,16, "S503%04X", (unsigned)address);
			cs = 3;
            cs += (unsigned char)(address >> 8);
            cs += (unsigned char)(address);
		}
        else if (type > 6)
        {
            if ((address >=0) && (address <= 0xFFFF))
            {
                type = 9;
                snprintf(tmp_buf,16, "S%d%02X%04X",type, 3,(unsigned int)address);
                cs = 3;
            }
            else if ((address > 0xFFFF) && (address <= 0xFFFFFF))
            {
                type = 8;
                snprintf(tmp_buf,16, "S%d%02X%06X",type, 4,(unsigned int)address);
                cs = 4;
            }
            else
            {
                type = 7;
                snprintf(tmp_buf,16, "S%d%02X%08X",type, 5,(unsigned int)address);
                cs = 5;
            }
            cs += (unsigned char)(address >> 24);
            cs += (unsigned char)(address >> 16);
            cs += (unsigned char)(address >> 8);
            cs += (unsigned char)(address);
            fputs(tmp_buf, fs);
        }
        cs = (0xFF - (cs & 0xFF)) & 0xFF;
        fprintf(fs, "%02X", cs);
        fputc('\n', fs);
    }
}

bool SRecordMem::WriteFile(char *filename, int byte_num_per_line, bool s5_recorded)
{
    bool ret = false;
    FILE  *fs = NULL;
    unsigned char header_buf[60];
    unsigned int i, j;
    int len;
	int srec_num;
    unsigned long address;
    unsigned char *pdata;
    fs = fopen(filename, "wt");
    if (fs != NULL)
    {
		ret = true;
        if (byte_num_per_line > 250)
        {
            byte_num_per_line = 250;
        }
        else if (byte_num_per_line <= 0)
        {
            byte_num_per_line = 32;
        }
        else
        {
            /* do nothing */
        }
        memcpy(&header_buf[0], m_header.m_module, 20);
        memcpy(&header_buf[20], m_header.m_ver, 2);
        memcpy(&header_buf[22], m_header.m_rev, 2);
        memcpy(&header_buf[24], m_header.m_comment, 36);
        WriteSrec(fs, 0, 0, header_buf, sizeof(header_buf));
		srec_num = 0;
        for (i=0; i<m_segIdx; i++)
        {
            j = 0;
            while (j<m_memBlock[i].m_dataLen)
            {
                len = ((m_memBlock[i].m_dataLen - j) > (unsigned int)byte_num_per_line) ? (unsigned int)byte_num_per_line : (m_memBlock[i].m_dataLen - j);
                address = m_memBlock[i].m_loadAddr + j;
                pdata = &m_memBlock[i].m_data[0] + j;
                WriteSrec(fs, 1, address, pdata, len);
                j+=len;
				srec_num ++;
            }
        }
		if (s5_recorded)
		{
			WriteSrec(fs, 5, srec_num, NULL, 0);
		}
        WriteSrec(fs, 9, m_startAddr, NULL, 0);
        fclose(fs);
    }
    return ret;
}

void SRecordMem::Reset(void)
{
    unsigned int i;
    memset(&m_header, 0, sizeof(m_header));
    m_startAddr = 0;
    m_segIdx = 0;
    for (i=0; i<m_memBlock.size(); i++)
    {
        m_memBlock[i].m_data.clear();
    }
    m_memBlock.clear();
}

unsigned int SRecordMem::AddSegment(unsigned int address, unsigned char *data, unsigned int data_len)
{
    unsigned int i;
    SRecMemBlock  sRecMemBlock;
    sRecMemBlock.m_loadAddr = address;
    sRecMemBlock.m_dataLen  = data_len;
    for (i=0; i<data_len; i++)
    {
        sRecMemBlock.m_data.push_back(data[i]);
    }
    m_memBlock.push_back( sRecMemBlock );
    m_segIdx = m_memBlock.size();
    return (m_segIdx-1);
}

unsigned int SRecordMem::AddData(unsigned int seg_idx, unsigned char *data, unsigned int data_len)
{
    unsigned int ret = 0;
    unsigned int i;
    if (seg_idx < m_memBlock.size())
    {
        for (i=0; i<data_len; i++)
        {
            m_memBlock[seg_idx].m_data.push_back(data[i]);
        }
        m_memBlock[seg_idx].m_dataLen = m_memBlock[seg_idx].m_data.size();
        ret = m_memBlock[seg_idx].m_dataLen;
    }
    return ret;
}
/* ---- Functions -------------------------------------------------------- */

SRecordMem* CreateSRecMemObject(void)
{
    return (new SRecordMem);
}
void DeleteSRecMemObject(SRecordMem *p)
{
    delete p;
}
