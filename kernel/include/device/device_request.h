#ifndef	_device_request_user_
#define	_device_request_user_

/* Module device_request */

#include <mach/kern_return.h>
#include <mach/port.h>
#include <mach/message.h>

#include <mach/std_types.h>
#include <device/device_types.h>
#include <device/net_status.h>

/* SimpleRoutine device_open_request */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t device_open_request
#if	defined(LINTLIBRARY)
    (device_server_port, reply_port, mode, name)
	mach_port_t device_server_port;
	mach_port_t reply_port;
	dev_mode_t mode;
	dev_name_t name;
{ return device_open_request(device_server_port, reply_port, mode, name); }
#else
(
	mach_port_t device_server_port,
	mach_port_t reply_port,
	dev_mode_t mode,
	dev_name_t name
);
#endif

/* SimpleRoutine device_write_request */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t device_write_request
#if	defined(LINTLIBRARY)
    (device, reply_port, mode, recnum, data, dataCnt)
	mach_port_t device;
	mach_port_t reply_port;
	dev_mode_t mode;
	recnum_t recnum;
	io_buf_ptr_t data;
	mach_msg_type_number_t dataCnt;
{ return device_write_request(device, reply_port, mode, recnum, data, dataCnt); }
#else
(
	mach_port_t device,
	mach_port_t reply_port,
	dev_mode_t mode,
	recnum_t recnum,
	io_buf_ptr_t data,
	mach_msg_type_number_t dataCnt
);
#endif

/* SimpleRoutine device_write_request_inband */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t device_write_request_inband
#if	defined(LINTLIBRARY)
    (device, reply_port, mode, recnum, data, dataCnt)
	mach_port_t device;
	mach_port_t reply_port;
	dev_mode_t mode;
	recnum_t recnum;
	io_buf_ptr_inband_t data;
	mach_msg_type_number_t dataCnt;
{ return device_write_request_inband(device, reply_port, mode, recnum, data, dataCnt); }
#else
(
	mach_port_t device,
	mach_port_t reply_port,
	dev_mode_t mode,
	recnum_t recnum,
	io_buf_ptr_inband_t data,
	mach_msg_type_number_t dataCnt
);
#endif

/* SimpleRoutine device_read_request */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t device_read_request
#if	defined(LINTLIBRARY)
    (device, reply_port, mode, recnum, bytes_wanted)
	mach_port_t device;
	mach_port_t reply_port;
	dev_mode_t mode;
	recnum_t recnum;
	int bytes_wanted;
{ return device_read_request(device, reply_port, mode, recnum, bytes_wanted); }
#else
(
	mach_port_t device,
	mach_port_t reply_port,
	dev_mode_t mode,
	recnum_t recnum,
	int bytes_wanted
);
#endif

/* SimpleRoutine device_read_request_inband */
#ifdef	mig_external
mig_external
#else
extern
#endif
kern_return_t device_read_request_inband
#if	defined(LINTLIBRARY)
    (device, reply_port, mode, recnum, bytes_wanted)
	mach_port_t device;
	mach_port_t reply_port;
	dev_mode_t mode;
	recnum_t recnum;
	int bytes_wanted;
{ return device_read_request_inband(device, reply_port, mode, recnum, bytes_wanted); }
#else
(
	mach_port_t device,
	mach_port_t reply_port,
	dev_mode_t mode,
	recnum_t recnum,
	int bytes_wanted
);
#endif

#endif	/* not defined(_device_request_user_) */
