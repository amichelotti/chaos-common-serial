#ifndef __posixserialcomm__h_
#define __posixserialcomm__h_
#ifdef __cplusplus
extern "C" {
#endif
  typedef int pserial_handle_t;

  /**
     Open and initialize a serial connection
     @param internal_buffering byte size of the internal buffers to be used
     @param serdev serial device to open
     @param baudrate baudrate of the serial connection
     @param parity (0=no parity, 1 = odd,2 even)
     @param bits (7 or 8)
     @param stop (1 or 2) stop bits
     @return a serial handle on success (the index corresponding to the serial line), negative otherwise
   */
  pserial_handle_t popen_serial(int internal_buffering,const char*serdev,int baudrate,int parity,int bits,int stop);

  /**
     Deinitialize and close a serial connection
     @param h a previously opened serial handle
     @return 0 if success
   */
  int pclose_serial(pserial_handle_t h);

  /**
     Schedule a write on the serial line of size bytes, no block
     @param h handle of the serial connection
     @param buf source buffer
     @param bsize byte size to transfer
     @return number of bytes successfully scheduled for write
   */
  int pwrite_async_serial(pserial_handle_t h, char*buf,int bsize);

  /**
     Schedule a read from the serial line of size bytes, no block
     @param h handle of the serial connection
     @param buf destination buffer
     @param bsize byte size to transfer
     @return number of bytes successfully read
   */
  int pread_async_serial(pserial_handle_t h, char*buf,int bsize);


  /**
     Write on the serial line of size bytes, blocks until transferred
     @param h handle of the serial connection
     @param buf source buffer
     @param bsize byte size to transfer
     @param timeo timeout in milliseconds (0 = no timeout)
     @param timoccur if set returns 1 if a timeout occurs, 0 otherwise
     @return number of bytes successfully written or negative if error occurs
   */
  int pwrite_serial(pserial_handle_t h, char*buf,int bsize,int timeo,int*timoccur);

  /**
     Schedule a read from the serial line of size bytes, blocks until transferred
     @param h handle of the serial connection
     @param buf destination buffer
     @param bsize byte size to transfer
     @param timeo timeout in milliseconds (0 = no timeout)
     @param timoccur if set returns 1 if a timeout occurs, 0 otherwise
     @return number of bytes successfully read or negative if error
   */
  int pread_serial(pserial_handle_t h, char*buf,int bsize,int timeo,int*timoccur);

  /**
     returns the number of bytes available for reading (in the internal buffer)
     @param h handle of the serial connection
     @return the number of bytes available
   */
  int byte_available_read_serial(pserial_handle_t h);

  /**
     returns the number of bytes wating to be trasmitted (in the internal buffer)
     @param h handle of the serial connection
     @return the number of bytes available
   */
  int byte_available_write_serial(pserial_handle_t h);
  

  /**
     Flush the output buffer (clears output buffers)
     @param h handle of the serial connection

   */
  void pwrite_flush_serial(pserial_handle_t h);

  /**
     Flush the input buffer 
     @param h handle of the serial connection
   */
  void pread_flush_serial(pserial_handle_t h);


    /**
     Write on the serial line of size bytes, blocks until transferred
     @param h handle of the serial connection
     @param buf source buffer
     @param bsize byte size to transfer
     @param timeo timeout in milliseconds (-1 = no timeout, 0=no blocking async)
     @param timoccur if set returns 1 if a timeout occurs, 0 otherwise
     @return number of bytes successfully written or negative if error occurs
   */
  int LVwrite_serial(pserial_handle_t h,void*buf,int bsize,int timeo,int*timoccur);
    /**
     Read on the serial line of size bytes, blocks until transferred
     @param h handle of the serial connection
     @param buf source/destination buffer
     @param bsize byte size to transfer
     @param timeo timeout in milliseconds (-1 = no timeout, 0=no blocking async)
     @param timoccur if set returns 1 if a timeout occurs, 0 otherwise
     @return number of bytes successfully read or negative if error occurs
     
   */
  int LVread_serial(pserial_handle_t h,void*buf,int bsize,int timeo,int*timoccur);

#ifdef __cplusplus
}
#endif

#endif
