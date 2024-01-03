/****************************************************************************/
/**
*
*  @file    SRecMem.h
*
*  @brief   Structure definitions used by the SRec2C program.
*
****************************************************************************/
/**
*  @defgroup   SRecMem   Structures used by the SRec2C program.
*
****************************************************************************/
#include <iostream>
#include <iomanip>
#include <vector>
#include <string>


#include "srec.h"

#if !defined( SRECMEM_H )
#define SRECMEM_H           ///< Include Guard

// ---- Include Files ------------------------------------------------------

/**
 * @addtogroup SRecMem
 * @{
 */

#ifdef __cplusplus
extern "C"
{
#endif
// ---- Constants and Types ------------------------------------------------

/**
 * Describes a single memory block
 */

typedef struct
{
   unsigned    m_loadAddr; ///< Address that data block should be loaded at.
   std::vector< unsigned char > m_data;    ///< Data buffer
   unsigned    m_dataLen;  ///< Number of bytes of data.
} SRecMemBlock;


// ---- Variable Externs ---------------------------------------------------
// ---- Function Prototypes ------------------------------------------------

/// @}

class SRecordMem : public SRecordParser
{
public:
            SRecordMem( void );
   virtual ~SRecordMem( void );

   virtual unsigned int GetSegmentNumber( void );
   virtual bool GetSegmentInfo(unsigned int seg_index, unsigned int *start_address, unsigned int *data_len);
   virtual unsigned int ReadSegmentData (unsigned int seg_index, unsigned int byte_offset, unsigned char *buff, unsigned int buf_size);
   virtual unsigned int GetData(unsigned int address, unsigned int size, unsigned char *buff, unsigned char pad_byte);
   virtual unsigned char *GetSegmentDataPointer (unsigned int seg_index, unsigned int *data_len);
   virtual void GetModuleName(char *module_name);
   virtual void SetModuleName(char *module_name);
   virtual void GetVersion(char *ver);
   virtual void SetVersion(char *ver);
   virtual void GetRevision(char *rev);
   virtual void SetRevision(char *rev);
   virtual void GetComment(char *comment);
   virtual void SetComment(char *comment);
   virtual bool WriteFile(char *filename, int byte_num_per_line = 32, bool s5_recorded = false);
   virtual void Reset(void);
   virtual unsigned int AddSegment(unsigned int address, unsigned char *data, unsigned int data_len);
   virtual unsigned int AddData(unsigned int seg_idx, unsigned char *data, unsigned int data_len);
   void PrintDebugInfo(bool en_print_byte);

protected:

   virtual  bool  FinishSegment( unsigned addr, unsigned len );
   virtual  bool  Header( const SRecordHeader *sRecHdr );
   virtual  bool  Data( const SRecordData *sRecData );
   virtual  bool  StartAddress( const SRecordData *sRecData );
   virtual  bool  StartSegment( unsigned addr );

private:
   SRecordHeader                 m_header;
   //template  class std::allocator<SRecMemBlock>;
   //template  class std::vector<SRecMemBlock, std::allocator<SRecMemBlock> >;
   std::vector< SRecMemBlock >   m_memBlock;
   unsigned                      m_startAddr;
   unsigned                      m_segIdx;
   void WriteSrec(FILE *fs, int type, unsigned long address, unsigned char *data, int len);
};

#ifdef __cplusplus
}
#endif

#endif // SRECMEM_H

