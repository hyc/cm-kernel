/* linux/arch/arm/mach-msm/board-surf7x30.c
 *
 * Copyright (C) 2010 Google, Inc.
 * Author: Dima Zavin <dima@android.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/mfd/pm8058.h>
#include <linux/platform_device.h>
#include <linux/usb/android_composite.h>

#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/setup.h>

#include <mach/board.h>
#include <mach/gpio.h>
#include <mach/hardware.h>
#include <mach/irqs.h>
#include <mach/msm_hsusb.h>
#include <mach/msm_iomap.h>
#include <mach/msm_ssbi.h>

#include "devices.h"
#include "proc_comm.h"

#define SURF7X30_PM8058_GPIO_BASE	FIRST_BOARD_GPIO
#define SURF7X30_PM8058_IRQ_BASE	FIRST_BOARD_IRQ

#define SURF7X30_GPIO_PMIC_INT_N	27

static struct resource smc91x_resources[] = {
	[0] = {
		.start	= 0x8A000300,
		.end	= 0x8A0003ff,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= MSM_GPIO_TO_INT(156),
		.end	= MSM_GPIO_TO_INT(156),
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device smc91x_device = {
	.name		= "smc91x",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(smc91x_resources),
	.resource	= smc91x_resources,
};

static int surf7x30_phy_init_seq[] = {
	0x0C, 0x31,
	0x31, 0x32,
	0x1D, 0x0D,
	0x1D, 0x10,
	-1 };

static void surf7x30_usb_phy_reset(void)
{
	u32 id;
	int ret;

	id = PCOM_CLKRGM_APPS_RESET_USB_PHY;
	ret = msm_proc_comm(PCOM_CLK_REGIME_SEC_RESET_ASSERT, &id, NULL);
	if (ret) {
		pr_err("%s: Cannot assert (%d)\n", __func__, ret);
		return;
	}

	msleep(1);

	id = PCOM_CLKRGM_APPS_RESET_USB_PHY;
	ret = msm_proc_comm(PCOM_CLK_REGIME_SEC_RESET_DEASSERT, &id, NULL);
	if (ret) {
		pr_err("%s: Cannot assert (%d)\n", __func__, ret);
		return;
	}
}

static void surf7x30_usb_hw_reset(bool enable)
{
	u32 id;
	int ret;
	u32 func;

	id = PCOM_CLKRGM_APPS_RESET_USBH;
	if (enable)
		func = PCOM_CLK_REGIME_SEC_RESET_ASSERT;
	else
		func = PCOM_CLK_REGIME_SEC_RESET_DEASSERT;
	ret = msm_proc_comm(func, &id, NULL);
	if (ret)
		pr_err("%s: Cannot set reset to %d (%d)\n", __func__, enable,
		       ret);
}

static struct msm_hsusb_platform_data msm_hsusb_pdata = {
	.phy_init_seq		= surf7x30_phy_init_seq,
	.phy_reset		= surf7x30_usb_phy_reset,
	.hw_reset		= surf7x30_usb_hw_reset,
};

static char *usb_functions[] = {
	"usb_mass_storage",
#ifdef CONFIG_USB_ANDROID_RNDIS
	"rndis",
#endif
#ifdef CONFIG_USB_ANDROID_ACM
	"acm",
#endif
};

static char *usb_functions_adb[] = {
	"usb_mass_storage",
	"adb",
#ifdef CONFIG_USB_ANDROID_RNDIS
	"rndis",
#endif
#ifdef CONFIG_USB_ANDROID_ACM
	"acm",
#endif
};

#ifdef CONFIG_USB_ANDROID_DIAG
static char *usb_functions_adb_diag[] = {
	"usb_mass_storage",
	"adb",
	"diag",
};
#endif

static char *usb_functions_all[] = {
	"usb_mass_storage",
	"adb",
#ifdef CONFIG_USB_ANDROID_ACM
	"acm",
#endif
#ifdef CONFIG_USB_ANDROID_RNDIS
	"rndis",
#endif
#ifdef CONFIG_USB_ANDROID_DIAG
	"diag",
#endif
};

static struct android_usb_product usb_products[] = {
	{
#ifdef CONFIG_USB_ANDROID_ACM
		.product_id	= 0x4e21,
#else
		.product_id	= 0x4e11,
#endif
		.num_functions	= ARRAY_SIZE(usb_functions),
		.functions	= usb_functions,
	},
	{
#ifdef CONFIG_USB_ANDROID_ACM
		.product_id	= 0x4e22,
#else
		.product_id	= 0x4e12,
#endif
		.num_functions	= ARRAY_SIZE(usb_functions_adb),
		.functions	= usb_functions_adb,
	},
#ifdef CONFIG_USB_ANDROID_DIAG
	{
		.product_id	= 0x4e17,
		.num_functions	= ARRAY_SIZE(usb_functions_adb_diag),
		.functions	= usb_functions_adb_diag,
	},
#endif
};

static struct usb_mass_storage_platform_data mass_storage_pdata = {
	.nluns		= 1,
	.vendor		= "Qualcomm, Inc.",
	.product	= "Surf7x30",
	.release	= 0x0100,
};

static struct platform_device usb_mass_storage_device = {
	.name	= "usb_mass_storage",
	.id	= -1,
	.dev	= {
		.platform_data = &mass_storage_pdata,
	},
};

static struct android_usb_platform_data android_usb_pdata = {
	.vendor_id	= 0x18d1,
	.product_id	= 0x4e11,
	.version	= 0x0100,
	.product_name		= "Surf7x30",
	.manufacturer_name	= "Qualcomm, Inc.",
	.num_products = ARRAY_SIZE(usb_products),
	.products = usb_products,
	.num_functions = ARRAY_SIZE(usb_functions_all),
	.functions = usb_functions_all,
};

static struct platform_device android_usb_device = {
	.name	= "android_usb",
	.id		= -1,
	.dev		= {
		.platform_data = &android_usb_pdata,
	},
};

static struct platform_device *devices[] __initdata = {
#if !defined(CONFIG_MSM_SERIAL_DEBUGGER)
	&msm_device_uart1,
#endif
	&msm_device_smd,
	&msm_device_nand,
	&msm_device_hsusb,
	&usb_mass_storage_device,
	&android_usb_device,
	&smc91x_device,
};

extern struct sys_timer msm_timer;

void msm_serial_debug_init(unsigned int base, int irq,
			   struct device *clk_device, int signal_irq);

static void __init surf7x30_init(void)
{
	printk("%s()\n", __func__);

#if defined(CONFIG_MSM_SERIAL_DEBUGGER)
	msm_serial_debug_init(MSM_UART1_PHYS, INT_UART1,
			      &msm_device_uart1.dev, 1);
#endif

	msm_device_hsusb.dev.platform_data = &msm_hsusb_pdata;
	platform_add_devices(devices, ARRAY_SIZE(devices));
	msm_hsusb_set_vbus_state(1);
	msm_hsusb_set_vbus_state(0);
	msm_hsusb_set_vbus_state(1);
}

static void __init surf7x30_fixup(struct machine_desc *desc, struct tag *tags,
				 char **cmdline, struct meminfo *mi)
{
	mi->nr_banks = 1;
	mi->bank[0].start = PHYS_OFFSET;
	mi->bank[0].node = PHYS_TO_NID(PHYS_OFFSET);
	mi->bank[0].size = (101*1024*1024);
}

static void __init surf7x30_map_io(void)
{
	msm_map_msm7x30_io();
	msm_clock_init(msm_clocks_7x30, msm_num_clocks_7x30);
}

static int surf7x30_pmic_init(void)
{
	int ret;
	u32 id;

	pr_info("%s()\n", __func__);
	id = PCOM_GPIO_CFG(SURF7X30_GPIO_PMIC_INT_N, 1, GPIO_INPUT,
			   GPIO_NO_PULL, GPIO_2MA);
	ret = msm_proc_comm(PCOM_RPC_GPIO_TLMM_CONFIG_EX, &id, 0);
	if (ret)
		pr_err("%s: gpio %d cfg failed\n", __func__,
		       SURF7X30_GPIO_PMIC_INT_N);
	return ret;
}

static struct pm8058_platform_data surf7x30_pm8058_pdata = {
	.irq_base	= SURF7X30_PM8058_IRQ_BASE,
	.gpio_base	= SURF7X30_PM8058_GPIO_BASE,
	.init		= surf7x30_pmic_init,
};

static struct msm_ssbi_platform_data surf7x30_ssbi_pmic_pdata = {
	.slave		= {
		.name		= "pm8058-core",
		.irq		= MSM_GPIO_TO_INT(SURF7X30_GPIO_PMIC_INT_N),
		.platform_data	= &surf7x30_pm8058_pdata,
	},
	.rspinlock_name	= "D:PMIC_SSBI",
};

static int __init surf7x30_ssbi_pmic_init(void)
{
	int ret;
	ret = gpiochip_reserve(surf7x30_pm8058_pdata.gpio_base,
			       PM8058_NUM_GPIOS);
	WARN(ret, "can't reserve pm8058 gpios. badness will ensue...\n");
	msm_device_ssbi_pmic.dev.platform_data = &surf7x30_ssbi_pmic_pdata;
	return platform_device_register(&msm_device_ssbi_pmic);
}
postcore_initcall(surf7x30_ssbi_pmic_init);

MACHINE_START(MSM7X30_SURF, "QCT SURF7X30 Development Board")
#ifdef CONFIG_MSM_DEBUG_UART
	.phys_io	= MSM_DEBUG_UART_PHYS,
	.io_pg_offst	= ((MSM_DEBUG_UART_BASE) >> 18) & 0xfffc,
#endif
	.boot_params	= 0x00200100,
	.fixup		= surf7x30_fixup,
	.map_io		= surf7x30_map_io,
	.init_irq	= msm_init_irq,
	.init_machine	= surf7x30_init,
	.timer		= &msm_timer,
MACHINE_END