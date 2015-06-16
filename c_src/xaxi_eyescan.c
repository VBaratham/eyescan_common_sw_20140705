/*
 * $Id: 
 *-
 *-   Purpose and Methods: 
 *-
 *-   Inputs  :
 *-   Outputs :
 *-   Controls:
 *-
 *-   Created  27-APR-2014   John D. Hobbs
 *- $Revision: $
 *-
*/

#include "assert.h"
#include "xbasic_types.h"
#include "xparameters.h"
#include "xaxi_eyescan.h"

static u32 eyescan_base =  XPAR_AXI_EYESCAN_OTC_0_BASEADDR;
static u32 eyescan_max  = XPAR_AXI_EYESCAN_OTC_0_HIGHADDR;

/*
 * The first set of routines are used to make a full address from the components for each
 * of the four access areas: global registers, common DRP, channel registers and
 * channel DRP.  This simply encapsulates these calculations in one place to avoid
 * errors.
 */

static u32* make_global_reg_address(u32 offset) {
    /* COULD ADD A RANGE CHECK HERE FOR THE OFFSET */
    u32 address = eyescan_base | (u32)XAXI_EYESCAN_GLOBAL_REG;
    address += 4*offset;
    return (u32*)address;
}

static u32* make_common_drp_address(u32 quadIdx, u32 offset) {
    /* COULD ADD RANGE CHECKS HERE FOR THE QUAD AND OFFSET */
    u32 address = eyescan_base | (u32)XAXI_EYESCAN_COMMON_DRP | (u32)(quadIdx << XAXI_EYESCAN_QUAD_SHIFT);
    address += 4*offset;
    return (u32*)address;
}

static u32* make_channel_reg_address(u32 chanIdx, u32 offset) {
    /* COULD ADD RANGE CHECKS HERE FOR THE CHANNEL AND OFFSET */
    u32 address = eyescan_base | (u32)XAXI_EYESCAN_CHANNEL_REG | (u32)(chanIdx << XAXI_EYESCAN_CHANNEL_SHIFT);
    address += 4*offset;
    return (u32*)address;
}

static u32* make_channel_drp_address(u32 chanIdx, u32 offset) {
    /* COULD ADD RANGE CHECKS HERE FOR THE CHANNEL AND OFFSET */
    u32 address = eyescan_base | (u32)XAXI_EYESCAN_CHANNEL_DRP | (u32)(chanIdx << XAXI_EYESCAN_CHANNEL_SHIFT);
    address += 4*offset;
    return (u32*)address;
}

/* ---------------------------------------------------------------------------
 * The next set are the user callable interface routines follow below,  There
 * is a read write pair for each of the four memory areas
 * ---------------------------------------------------------------------------*/
u32 xaxi_eyescan_read_global(u32 offset) {
    u32 *address = make_global_reg_address(offset);
    return xaxi_eyescan_read(address);
}

void xaxi_eyescan_write_global(u32 offset, u32 value) {
    u32 *address = make_global_reg_address(offset);
    return xaxi_eyescan_write(address,value);

}

/* The common (quad) DRP routines */
u32 xaxi_eyescan_read_common_drp(u32 quadIdx, u32 drpOffset) {
    u32 *address = make_common_drp_address(quadIdx,drpOffset);
    return xaxi_eyescan_read(address);
}

void xaxi_eyescan_write_common_drp(u32 quadIdx, u32 drpOffset, u32 value){
    u32 *address = make_common_drp_address(quadIdx,drpOffset);
    return xaxi_eyescan_write(address,value);
}

/* The channel register routines */
u32 xaxi_eyescan_read_channel_reg(u32 chanIdx, u32 chanRegOffset){
    u32 *address = make_channel_reg_address(chanIdx, chanRegOffset);
    return xaxi_eyescan_read(address);
}

void xaxi_eyescan_write_channel_reg(u32 chanIdx, u32 chanRegOffset, u32 value){
    u32 *address = make_channel_reg_address(chanIdx, chanRegOffset);
    return xaxi_eyescan_write(address,value);
}

/* The channel DRP routines.  These are the workhorse routines. */
u32 xaxi_eyescan_read_channel_drp(u32 chanIdx, u32 drpOffset) {
    u32 *address = make_channel_drp_address(chanIdx, drpOffset);
    return xaxi_eyescan_read(address);
}

void xaxi_eyescan_write_channel_drp(u32 chanIdx, u32 drpOffset, u32 value) {
    u32 *address = make_channel_drp_address(chanIdx, drpOffset);
    return xaxi_eyescan_write(address,value);
}

/* ------------------------------------------------------------------------
 * The following two routines are the fundamental I/O routines which always
 * should eventually get called.
 * -------------------------------------------------------------------------*/
u32 xaxi_eyescan_read(u32 *address) {
    assert( (((u32)address) & (u32)3) == 0);  /* Must be 32 bit aligned */
    assert( address >= (u32*)eyescan_base);
    assert( address <= (u32*)eyescan_max );
    return *address;
}

void xaxi_eyescan_write(u32 *address, u32 value) {
    assert( (((u32)address) & (u32)3) == 0);  /* Must be 32 bit aligned */
    assert( address >= (u32*)eyescan_base);
    assert( address <= (u32*)eyescan_max );
    *address = value;
}
