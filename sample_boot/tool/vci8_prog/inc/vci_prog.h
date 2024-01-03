#ifndef VCI_PROG_H
#define VCI_PROG_H

#ifdef __cplusplus
extern "C" {
#endif

#define VCI_PROG_STS_OK                         (0)
#define VCI_PROG_ERR_INVALID_ARG                (-1)
#define VCI_PROG_ERR_OPEN_FILE_FAIL             (-2)
#define VCI_PROG_ERR_OPEN_SOCKET_FAIL           (-3)
#define VCI_PROG_ERR_ENTER_BOOT_FAIL            (-4)
#define VCI_PROG_ERR_ENTER_PROG_SESSION_FAIL    (-5)
#define VCI_PROG_ERR_SEC_ACCESS_FAIL            (-6)
#define VCI_PROG_ERR_WRITE_ENC_KEY_FAIL         (-7)
#define VCI_PROG_ERR_ERASE_MEMORY_FAIL          (-8)
#define VCI_PROG_ERR_READ_SREC_FAIL             (-9)
#define VCI_PROG_ERR_DOWNLOAD_DATA_FAIL         (-10)
#define VCI_PROG_ERR_EXIT_DOWNLOAD_FAIL         (-11)
#define VCI_PROG_ERR_CHECKSUM_VALIDATE_FAIL     (-12)
#define VCI_PROG_ERR_RESET_DEVICE_FAIL          (-13)

typedef void *vci_prog_callback_t(int total, int prog);

int vci_prog(char *ip_addr, char *file_name, vci_prog_callback_t callback);

#ifdef __cplusplus
}
#endif

#endif