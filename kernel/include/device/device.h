#ifndef	_device_user_
#define	_device_user_

/* Module device */

#include <mach/kern_return.h>
#include <mach/port.h>
#include <mach/message.h>

#include <mach/std_types.h>
#include <mach/mach_types.h>
#include <device/device_types.h>
#include <device/net_status.h>

/* Routine device_open */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t device_open
#if	defined(LINTLIBRARY)
    (master_port, mode, name, device)
	mach_port_t master_port;
	dev_mode_t mode;
	dev_name_t name;
	mach_port_t *device;
{ return device_open(master_port, mode, name, device); }
#else
(
	mach_port_t master_port,
	dev_mode_t mode,
	dev_name_t name,
	mach_port_t *device
);
#endif

/* Routine device_close */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t device_close
#if	defined(LINTLIBRARY)
    (device)
	mach_port_t device;
{ return device_close(device); }
#else
(
	mach_port_t device
);
#endif

/* Routine device_write */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t device_write
#if	defined(LINTLIBRARY)
    (device, mode, recnum, data, dataCnt, bytes_written)
	mach_port_t device;
	dev_mode_t mode;
	recnum_t recnum;
	io_buf_ptr_t data;
	mach_msg_type_number_t dataCnt;
	int *bytes_written;
{ return device_write(device, mode, recnum, data, dataCnt, bytes_written); }
#else
(
	mach_port_t device,
	dev_mode_t mode,
	recnum_t recnum,
	io_buf_ptr_t data,
	mach_msg_type_number_t dataCnt,
	int *bytes_written
);
#endif

/* Routine device_write_inband */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t device_write_inband
#if	defined(LINTLIBRARY)
    (device, mode, recnum, data, dataCnt, bytes_written)
	mach_port_t device;
	dev_mode_t mode;
	recnum_t recnum;
	io_buf_ptr_inband_t data;
	mach_msg_type_number_t dataCnt;
	int *bytes_written;
{ return device_write_inband(device, mode, recnum, data, dataCnt, bytes_written); }
#else
(
	mach_port_t device,
	dev_mode_t mode,
	recnum_t recnum,
	io_buf_ptr_inband_t data,
	mach_msg_type_number_t dataCnt,
	int *bytes_written
);
#endif

/* Routine device_read */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t device_read
#if	defined(LINTLIBRARY)
    (device, mode, recnum, bytes_wanted, data, dataCnt)
	mach_port_t device;
	dev_mode_t mode;
	recnum_t recnum;
	int bytes_wanted;
	io_buf_ptr_t *data;
	mach_msg_type_number_t *dataCnt;
{ return device_read(device, mode, recnum, bytes_wanted, data, dataCnt); }
#else
(
	mach_port_t device,
	dev_mode_t mode,
	recnum_t recnum,
	int bytes_wanted,
	io_buf_ptr_t *data,
	mach_msg_type_number_t *dataCnt
);
#endif

/* Routine device_read_inband */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t device_read_inband
#if	defined(LINTLIBRARY)
    (device, mode, recnum, bytes_wanted, data, dataCnt)
	mach_port_t device;
	dev_mode_t mode;
	recnum_t recnum;
	int bytes_wanted;
	io_buf_ptr_inband_t data;
	mach_msg_type_number_t *dataCnt;
{ return device_read_inband(device, mode, recnum, bytes_wanted, data, dataCnt); }
#else
(
	mach_port_t device,
	dev_mode_t mode,
	recnum_t recnum,
	int bytes_wanted,
	io_buf_ptr_inband_t data,
	mach_msg_type_number_t *dataCnt
);
#endif

/* Routine xxx_device_set_status */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t xxx_device_set_status
#if	defined(LINTLIBRARY)
    (device, flavor, status, statusCnt)
	mach_port_t device;
	dev_flavor_t flavor;
	dev_status_t status;
	mach_msg_type_number_t statusCnt;
{ return xxx_device_set_status(device, flavor, status, statusCnt); }
#else
(
	mach_port_t device,
	dev_flavor_t flavor,
	dev_status_t status,
	mach_msg_type_number_t statusCnt
);
#endif

/* Routine xxx_device_get_status */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t xxx_device_get_status
#if	defined(LINTLIBRARY)
    (device, flavor, status, statusCnt)
	mach_port_t device;
	dev_flavor_t flavor;
	dev_status_t status;
	mach_msg_type_number_t *statusCnt;
{ return xxx_device_get_status(device, flavor, status, statusCnt); }
#else
(
	mach_port_t device,
	dev_flavor_t flavor,
	dev_status_t status,
	mach_msg_type_number_t *statusCnt
);
#endif

/* Routine xxx_device_set_filter */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t xxx_device_set_filter
#if	defined(LINTLIBRARY)
    (device, receive_port, receive_portPoly, priority, filter, filterCnt)
	mach_port_t device;
	mach_port_t receive_port;
	mach_msg_type_name_t receive_portPoly;
	int priority;
	filter_array_t filter;
	mach_msg_type_number_t filterCnt;
{ return xxx_device_set_filter(device, receive_port, receive_portPoly, priority, filter, filterCnt); }
#else
(
	mach_port_t device,
	mach_port_t receive_port,
	mach_msg_type_name_t receive_portPoly,
	int priority,
	filter_array_t filter,
	mach_msg_type_number_t filterCnt
);
#endif

/* Routine device_map */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t device_map
#if	defined(LINTLIBRARY)
    (device, prot, offset, size, pager, unmap)
	mach_port_t device;
	vm_prot_t prot;
	vm_offset_t offset;
	vm_size_t size;
	mach_port_t *pager;
	int unmap;
{ return device_map(device, prot, offset, size, pager, unmap); }
#else
(
	mach_port_t device,
	vm_prot_t prot,
	vm_offset_t offset,
	vm_size_t size,
	mach_port_t *pager,
	int unmap
);
#endif

/* Routine device_set_status */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t device_set_status
#if	defined(LINTLIBRARY)
    (device, flavor, status, statusCnt)
	mach_port_t device;
	dev_flavor_t flavor;
	dev_status_t status;
	mach_msg_type_number_t statusCnt;
{ return device_set_status(device, flavor, status, statusCnt); }
#else
(
	mach_port_t device,
	dev_flavor_t flavor,
	dev_status_t status,
	mach_msg_type_number_t statusCnt
);
#endif

/* Routine device_get_status */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t device_get_status
#if	defined(LINTLIBRARY)
    (device, flavor, status, statusCnt)
	mach_port_t device;
	dev_flavor_t flavor;
	dev_status_t status;
	mach_msg_type_number_t *statusCnt;
{ return device_get_status(device, flavor, status, statusCnt); }
#else
(
	mach_port_t device,
	dev_flavor_t flavor,
	dev_status_t status,
	mach_msg_type_number_t *statusCnt
);
#endif

/* Routine device_set_filter */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t device_set_filter
#if	defined(LINTLIBRARY)
    (device, receive_port, receive_portPoly, priority, filter, filterCnt)
	mach_port_t device;
	mach_port_t receive_port;
	mach_msg_type_name_t receive_portPoly;
	int priority;
	filter_array_t filter;
	mach_msg_type_number_t filterCnt;
{ return device_set_filter(device, receive_port, receive_portPoly, priority, filter, filterCnt); }
#else
(
	mach_port_t device,
	mach_port_t receive_port,
	mach_msg_type_name_t receive_portPoly,
	int priority,
	filter_array_t filter,
	mach_msg_type_number_t filterCnt
);
#endif

#endif	/* not defined(_device_user_) */
