#include <linux/module.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/bitops.h>
#include <linux/of_address.h>
#include <soc_crgperiph_interface.h>
#include <soc_pctrl_interface.h>
#include <soc_usb31_misc_ctrl_interface.h>

#include "dwc3-hisi.h"
#include "hisi_usb3_misctrl.h"
#include "hisi_usb3_31phy.h"
#include "common.h"
#include "combophy_regcfg.h"

#define USB31PHY_CR_ACK			BIT(7)
#define USB31PHY_CR_WR_EN		BIT(5)
#define USB31PHY_CR_RD_EN		BIT(3)
#define USB31PHY_CR_SEL			BIT(4)
#define USB31PHY_CR_CLK			BIT(2)

#define USB31PHY_CR_ADDR_MASK		GENMASK(31, 16)
#define USB31PHY_CR_DATA_RD_START	16

/* MISC CTRL */
#define USB_MISC_REG_PHY_CR_PARA_CTRL	0x54
#define USB_MISC_REG_PHY_CR_PARA_DATA	0x58

#define combophy_to_hisi_usb3_31phy(ptr) container_of(ptr, struct hisi_usb3_31phy, combophy)

struct hisi_usb3_31phy {
	struct hisi_usb_combophy combophy;
	void __iomem *usb3_misc_ctrl_base;
};

static struct hisi_usb3_31phy *_hisi_usb3_31phy;

static int usb31phy_cr_ack_wait_n = 100000;

struct hisi_usb_combophy *usb3_get_combophy_phandle(void)
{
	if (_hisi_usb3_31phy)
		return &_hisi_usb3_31phy->combophy;

	return NULL;
}

#ifdef DEBUG_USB31PHY
void usb31phy_cr_dbg_setacktime(int n)
{
	usb31phy_cr_ack_wait_n = n;
}
#endif

static void usb31phy_cr_set_addr(u32 addr)
{
	volatile u32 tmp;
	tmp = usb3_misc_reg_readl(USB_MISC_REG_PHY_CR_PARA_CTRL);
	tmp &= ~USB31PHY_CR_ADDR_MASK; /*lint !e648 */
	tmp |= ((addr << 16) & ~GENMASK(15, 0)); /*lint !e647 */
	usb3_misc_reg_writel(tmp, USB_MISC_REG_PHY_CR_PARA_CTRL);
}

static void usb31phy_cr_clk(void)
{
	volatile u32 tmp;

	/* clk up */
	tmp = usb3_misc_reg_readl(USB_MISC_REG_PHY_CR_PARA_CTRL);
	tmp |= USB31PHY_CR_CLK;
	usb3_misc_reg_writel(tmp, USB_MISC_REG_PHY_CR_PARA_CTRL);

	/* clk down */
	tmp = usb3_misc_reg_readl(USB_MISC_REG_PHY_CR_PARA_CTRL);
	tmp &= ~USB31PHY_CR_CLK;
	usb3_misc_reg_writel(tmp, USB_MISC_REG_PHY_CR_PARA_CTRL);
}

static void usb31phy_cr_clk_toogle(unsigned count)
{
	unsigned i = 0;

	for(i = 0; i < count; i++) {
		usb31phy_cr_clk();
	}
}

static void usb31phy_cr_100clk(void)
{
	usb31phy_cr_clk_toogle(100);
}

static int usb31phy_cr_wait_ack(void)
{
	int32_t i = usb31phy_cr_ack_wait_n;
	int ret = 0;

	while (!(usb3_misc_reg_readl(USB_MISC_REG_PHY_CR_PARA_CTRL) & USB31PHY_CR_ACK)) {
		usb31phy_cr_clk();

		if(0 == i) {
			pr_err("wait phy cr timeout! %lu, %u\n",
					USB31PHY_CR_ACK,
					usb3_misc_reg_readl(USB_MISC_REG_PHY_CR_PARA_CTRL));
			ret = 1;
			break;
		}
		i--;
	}

	return ret;
}

enum phy_cr_start_type {
	PHY_CR_READ,
	PHY_CR_WRITE
};

static void usb31phy_cr_start(enum phy_cr_start_type type)
{
	volatile u32 high, low;

	high = usb3_misc_reg_readl(USB_MISC_REG_PHY_CR_PARA_CTRL);

	if (PHY_CR_READ == type) {
		high |= USB31PHY_CR_RD_EN;
	} else if(PHY_CR_WRITE == type) {
		high |= USB31PHY_CR_WR_EN;
	} else {
		pr_err("unknow phy cr type:%d\n", type);
		return;
	}

	usb3_misc_reg_writel(high, USB_MISC_REG_PHY_CR_PARA_CTRL);

	usb31phy_cr_clk();

	/* read & write */
	low = usb3_misc_reg_readl(USB_MISC_REG_PHY_CR_PARA_CTRL);
	low &= ~USB31PHY_CR_RD_EN;
	low &= ~USB31PHY_CR_WR_EN;
	usb3_misc_reg_writel(low, USB_MISC_REG_PHY_CR_PARA_CTRL);
}

static void usb31phy_cr_set_sel(void)
{
	volatile u32 temp;

	temp = usb3_misc_reg_readl(USB_MISC_REG_PHY_CR_PARA_CTRL);

	temp |= USB31PHY_CR_SEL;

	usb3_misc_reg_writel(temp, USB_MISC_REG_PHY_CR_PARA_CTRL);
}

u32 usb31phy_cr_read(u32 addr)
{
	volatile u32 data;

	usb31phy_cr_100clk();

	/* step-0: set cr para selk */
	usb31phy_cr_set_sel();

	/* step-1: set addr for read */
	usb31phy_cr_set_addr(addr);

	/* step-2: send read start cmd */
	usb31phy_cr_start(PHY_CR_READ);

	/* step-3: wait cmd ack */
	if (usb31phy_cr_wait_ack()) {
		pr_err("wait ack timeout!\n");
	}

	/* ack, read data now*/
	data = usb3_misc_reg_readl(USB_MISC_REG_PHY_CR_PARA_DATA);
	return ((data >> USB31PHY_CR_DATA_RD_START) & GENMASK(15, 0));
}

int usb31phy_cr_write(u32 addr, u32 value)
{
	int ret = 0;

	usb31phy_cr_100clk();

	/* step-0: set cr para selk */
	usb31phy_cr_set_sel();

	/* step-1: set addr for write */
	usb31phy_cr_set_addr(addr);

	/* step-2: set data for write */
	usb3_misc_reg_writel(value, USB_MISC_REG_PHY_CR_PARA_DATA);

	/* step-3: send write start cmd */
	usb31phy_cr_start(PHY_CR_WRITE);

	/* wait cmd ack if write success */
	if (usb31phy_cr_wait_ack()) {
		ret = 1;
	}

	return ret;
}


static int usb31phy_cr_fast_write(u32 addr, u32 value)
{
	int ret = 0;
	u32 tmp = usb3_misc_reg_readl(USB_MISC_REG_PHY_CR_PARA_CTRL);
	int i;

	for(i = 0; i < 32; i++) {
		/* clk up */
		tmp |= USB31PHY_CR_CLK;
		usb3_misc_reg_writel(tmp, USB_MISC_REG_PHY_CR_PARA_CTRL);

		/* clk down */
		tmp &= ~USB31PHY_CR_CLK;
		usb3_misc_reg_writel(tmp, USB_MISC_REG_PHY_CR_PARA_CTRL);
	}

	/* step-0: set cr para selk */
	usb31phy_cr_set_sel();

	/* step-1: set addr for write */
	usb31phy_cr_set_addr(addr);

	/* step-2: set data for write */
	usb3_misc_reg_writel(value, USB_MISC_REG_PHY_CR_PARA_DATA);

	/* step-3: send write start cmd */
	usb31phy_cr_start(PHY_CR_WRITE);

	/* wait cmd ack if write success */
	if (usb31phy_cr_wait_ack()) {
		ret = 1;
	}

	return ret;
}

static void usb31phy_cr_32clk(void)
{
	usb31phy_cr_clk_toogle(32);
}

static void _reset_phy(struct hisi_usb_combophy *combophy)
{
	combophy_regcfg_phyreset();
}

static void _reset_misc_ctrl(struct hisi_usb_combophy *combophy)
{
	combophy_regcfg_reset_misc();
}

static void _unreset_misc_ctrl(struct hisi_usb_combophy *combophy)
{
	combophy_regcfg_unreset_misc();
}

#ifdef COMBOPHY_FW_UPDATE
#include "firmware.h"

static void _combophy_firmware_write(struct hisi_usb_combophy *combophy)
{
	int i, cnt;
	int fw_size = sizeof(firmware)/sizeof(firmware[0]);
	void __iomem *usb3_misc_ctrl_base;
	struct hisi_usb3_31phy *usb3_31phy = NULL;

	if (!combophy) {
		usb_err("combophy is NULL\n");
		return;
	}

	usb3_31phy = combophy_to_hisi_usb3_31phy(combophy);
	usb3_misc_ctrl_base = usb3_31phy->usb3_misc_ctrl_base;

	usb_info("fw_size[%d]\n", fw_size);

	/*选择CR 接口： MISC54[4] =  1*/
	set_bits(BIT(SOC_USB31_MISC_CTRL_USB_MISC_CFG54_usb3_phy0_cr_para_sel_START),
		SOC_USB31_MISC_CTRL_USB_MISC_CFG54_ADDR(usb3_misc_ctrl_base));
	/*toggle clock * 32次： MISC54[2] =  1； MISC54[2] =  0；循环32次*/
	usb31phy_cr_32clk();

	/*
	3、等待PHY准备好
	 wait for sram_init_done：MISC5c[12]  ==1
	*/
	cnt = 20;
	while(cnt--) {
		if(is_bits_set(BIT(SOC_USB31_MISC_CTRL_USB_MISC_CFG5C_phy0_sram_init_done_START),
				SOC_USB31_MISC_CTRL_USB_MISC_CFG5C_ADDR(usb3_misc_ctrl_base)))
				break;
		udelay(500);
		usb31phy_cr_32clk();
	}
	if (cnt <= 0)
		pr_info("[%s]wait sram init done cnt %d\n", __func__, cnt);

	/*
	4、更新firmware:
	将获得的firmware，依次写入（调用CR写函数）
	*/
	i = 0;
	usb31phy_cr_write(0xc000+i,firmware[i]); /* fist write after system startup always failed */
	(void)usb31phy_cr_read(0x0); /* check pipe clock ready */
	for(i = 0; i< fw_size; i++) {
		usb31phy_cr_fast_write(0xc000+i,firmware[i]);
	}

	/*toggle clock * 32次： MISC54[2] =  1； MISC54[2] =  0；循环32次*/
	usb31phy_cr_32clk();

	set_bits(BIT(SOC_USB31_MISC_CTRL_USB_MISC_CFG54_usb3_phy0_cr_para_ack_START),
		SOC_USB31_MISC_CTRL_USB_MISC_CFGB4_ADDR(usb3_misc_ctrl_base));

	/*5、通知PHY读取数据
	sram_ext_ld_done =1: MISC5c[3]  =1*/
	set_bits(BIT(SOC_USB31_MISC_CTRL_USB_MISC_CFG54_usb3_phy0_cr_para_rd_en_START),
		SOC_USB31_MISC_CTRL_USB_MISC_CFG5C_ADDR(usb3_misc_ctrl_base));

	/*toggle clock * 32次： MISC54[2] =  1； MISC54[2] =  0；循环32次*/
	usb31phy_cr_32clk();

	/*6、延迟1mS，等PHY OK*/
	msleep(1);
}

static void _combophy_firmware_update_prepare(struct hisi_usb_combophy *combophy)
{
	struct hisi_usb3_31phy *usb3_31phy = NULL;

	if (!combophy) {
		usb_err("combophy is NULL\n");
		return;
	}

	usb3_31phy = combophy_to_hisi_usb3_31phy(combophy);
	clr_bits(BIT(SOC_USB31_MISC_CTRL_USB_MISC_CFG5C_usb3_phy0_sram_bypass_START),
			SOC_USB31_MISC_CTRL_USB_MISC_CFG5C_ADDR(usb3_31phy->usb3_misc_ctrl_base));
}
#endif

static int __init hisi_usb3_31phy_module_init(void)
{
	struct device_node *np = NULL;
	int ret;

	_hisi_usb3_31phy = kzalloc(sizeof(*_hisi_usb3_31phy), GFP_KERNEL);
	if (!_hisi_usb3_31phy)
		return -ENOMEM;

	/*
	 * map usb3 misc ctrl region
	 */
	np = of_find_compatible_node(NULL, NULL, "hisilicon,usb3otg_bc");
	if (!np) {
		usb_err("get peri cfg node failed!\n");
		ret = -EINVAL;
		goto err_free;
	}

	_hisi_usb3_31phy->usb3_misc_ctrl_base = of_iomap(np, 0);
	if (!_hisi_usb3_31phy->usb3_misc_ctrl_base) {
		ret = -ENOMEM;
		goto err_free;
	}

#ifdef COMBOPHY_FW_UPDATE
	_hisi_usb3_31phy->combophy.firmware_update_prepare =
		_combophy_firmware_update_prepare;
	_hisi_usb3_31phy->combophy.firmware_update = _combophy_firmware_write;
#endif
	_hisi_usb3_31phy->combophy.reset_phy = _reset_phy;
	_hisi_usb3_31phy->combophy.reset_misc_ctrl = _reset_misc_ctrl;
	_hisi_usb3_31phy->combophy.unreset_misc_ctrl = _unreset_misc_ctrl;

	return 0;

err_free:
	kfree(_hisi_usb3_31phy);
	_hisi_usb3_31phy = NULL;
	return ret;
}

static void __exit hisi_usb3_31phy_module_exit(void)
{
	iounmap(_hisi_usb3_31phy->usb3_misc_ctrl_base);
	kfree(_hisi_usb3_31phy);
	_hisi_usb3_31phy = NULL;
}

module_init(hisi_usb3_31phy_module_init);
module_exit(hisi_usb3_31phy_module_exit);
