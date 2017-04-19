

#include "iqs128.h"
#include <linux/types.h>
#include <linux/switch.h>
#include <linux/seq_file.h>
#include <linux/proc_fs.h>
#include <linux/interrupt.h>
#include <gpio_const.h>
#include <linux/pinctrl/consumer.h>
#include <linux/platform_device.h>
#include <linux/irq.h>
#include <linux/of_irq.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/kobject.h>
#include <linux/delay.h>

struct switch_dev *iqs128_switch_dev = NULL;
int iqs128_irq;
unsigned int iqs128_eint_gpio;
unsigned int iqs128_eint_debounce;
static int iqs128_int_value = 0;
static struct workqueue_struct *iqs128_wq;
struct work_struct *iqs128_work;

struct pinctrl *iqs128_eint_pinctrl;
struct pinctrl_state *iqs128_eint_default;
struct pinctrl_state *iqs128_eint_active;
struct pinctrl_state *iqs128_power_en_enable;
struct pinctrl_state *iqs128_power_en_disable;

struct of_device_id iqs128_of_match[] = {
    {.compatible = "mediatek, mt6580-psensor"},
    {},
};

struct proc_dir_entry *proc_psensor_dir = NULL;
static struct proc_dir_entry *proc_name = NULL;
u8 psensor_power_on  = 0;

void send_psensor_uevent(enum PSENSOR_STATUS val);
extern int mt_get_gpio_in(unsigned long pin);

//create power_en interface start
ssize_t psensor_power_en_store(struct file *file, const char __user *buf, size_t size, loff_t *ppos) {
    if (!strncmp(buf, "on", 2) && (psensor_power_on == 0)) {
        pinctrl_select_state(iqs128_eint_pinctrl, iqs128_power_en_enable);
        msleep(3);
        //pinctrl_select_state(iqs128_eint_pinctrl, iqs128_eint_active);
        irq_set_irq_type(iqs128_irq, mt_get_gpio_in(iqs128_eint_gpio | 0x80000000) ? IRQF_TRIGGER_LOW : IRQF_TRIGGER_HIGH);
        enable_irq(iqs128_irq);
        psensor_power_on = 1;
        printk(KERN_CRIT"%s: Psensor power on\n", __func__);
    } else if (!strncmp(buf, "off", 3) && (psensor_power_on == 1)) {
        //pinctrl_select_state(iqs128_eint_pinctrl, iqs128_eint_default);
        disable_irq(iqs128_irq);
        msleep(1);
        pinctrl_select_state(iqs128_eint_pinctrl, iqs128_power_en_disable);
        psensor_power_on = 0;
        send_psensor_uevent(PSENSOR_STATUS_FAR);
        printk(KERN_CRIT"%s: Psensor power off\n", __func__);
    }

    return size;
}

static int psensor_power_en_show(struct seq_file *m, void *v) {
    if (psensor_power_on == 1) {
        seq_printf(m, "on\n");
    } else if (psensor_power_on == 0) {
        seq_printf(m, "off\n");
    }
    printk("iqs128_int_value  = %d  psensor_power_on = %d \n", mt_get_gpio_in(iqs128_eint_gpio | 0x80000000), psensor_power_on);
    return 0;
}

static int psensor_power_en_open(struct inode *inode, struct file *file) {
    return single_open(file, psensor_power_en_show, NULL);
}

static const struct file_operations psensor_power_en_fops = {
    .open = psensor_power_en_open,
    .write = psensor_power_en_store,
    .read = seq_read,
    .owner = THIS_MODULE,
};
//create power_en interface end

void send_psensor_uevent(enum PSENSOR_STATUS val) {
    char *envp[2];
    char psensor[20];

    if(iqs128_switch_dev == NULL) {
        printk(KERN_CRIT"%s: iqs128 switch dev is NULL!\n", __func__);
        return;
    }
    iqs128_switch_dev->state = val;
    snprintf(psensor, sizeof(psensor), "SWITCH_STATE=%d", iqs128_switch_dev->state);
    envp[0] = psensor;
    envp[1] = NULL;

    if(iqs128_switch_dev->state == 0)	//near
        kobject_uevent_env(&iqs128_switch_dev->dev->kobj, KOBJ_ADD, envp);
    else					//far
        kobject_uevent_env(&iqs128_switch_dev->dev->kobj, KOBJ_REMOVE, envp);
}

int iqs128_pinctrl_init(struct device *dev) {
    int ret = 0;
    iqs128_eint_pinctrl = devm_pinctrl_get(dev);
    if (IS_ERR(iqs128_eint_pinctrl)) {
        ret = PTR_ERR(iqs128_eint_pinctrl);
        printk(KERN_CRIT"%s: get iqs128_eint_pinctrl failed!\n", __func__);
        return ret;
    }

    iqs128_eint_default = pinctrl_lookup_state(iqs128_eint_pinctrl, "default");
    if (IS_ERR(iqs128_eint_default)) {
        ret = PTR_ERR(iqs128_eint_default);
        printk(KERN_CRIT"%s: get iqs128_eint_default failed!\n", __func__);
        return ret;
    }

    iqs128_eint_active = pinctrl_lookup_state(iqs128_eint_pinctrl, "active");
    if (IS_ERR(iqs128_eint_active)) {
        ret = PTR_ERR(iqs128_eint_active);
        printk(KERN_CRIT"%s: get iqs128_eint_active failed!\n", __func__);
        return ret;
    }

    iqs128_power_en_enable = pinctrl_lookup_state(iqs128_eint_pinctrl, "power_en_enable");
    if (IS_ERR(iqs128_power_en_enable)) {
        ret = PTR_ERR(iqs128_power_en_enable);
        printk(KERN_CRIT"%s: get iqs128_power_en_enable failed!\n", __func__);
        return ret;
    }

    iqs128_power_en_disable = pinctrl_lookup_state(iqs128_eint_pinctrl, "power_en_disable");
    if (IS_ERR(iqs128_power_en_disable)) {
        ret = PTR_ERR(iqs128_power_en_disable);
        printk(KERN_CRIT"%s: get iqs128_power_en_disable failed!\n", __func__);
        return ret;
    }

    return 0;
}

static void do_iqs128_work(struct work_struct *work) {
    printk("send_psensor_uevent = %d\n", iqs128_int_value);
    send_psensor_uevent(iqs128_int_value);
}

static irqreturn_t iqs128_int_handler(void) {
    //printk(KERN_CRIT"%s: receive interrupt!\n", __func__);
    iqs128_int_value = mt_get_gpio_in(iqs128_eint_gpio | 0x80000000);
    irq_set_irq_type(iqs128_irq, iqs128_int_value ? IRQF_TRIGGER_LOW : IRQF_TRIGGER_HIGH);
    gpio_set_debounce(iqs128_eint_gpio, iqs128_eint_debounce);
    queue_work(iqs128_wq, iqs128_work);

    return IRQ_HANDLED;
}

static int iqs128_probe(struct platform_device *dev) {
    int ret = 0;
    struct device_node *node = NULL;
    unsigned int ints[2] = { 0, 0 };

    printk(KERN_CRIT"%s: start!\n", __func__);

    //regiter switch dev start
    iqs128_switch_dev = kzalloc(sizeof(struct switch_dev), GFP_KERNEL);
    if (iqs128_switch_dev == NULL) {
        printk(KERN_CRIT"%s: iqs128 switch dev alloc failed!\n", __func__);
        return -1;
    }
    iqs128_switch_dev->name = "psensor";
    iqs128_switch_dev->state = PSENSOR_STATUS_FAR;

    ret = switch_dev_register(iqs128_switch_dev);
    if (ret < 0) {
        printk(KERN_CRIT"%s: iqs128 switch dev regiser failed!\n", __func__);
        goto switch_dev_register_failed;
    }
    send_psensor_uevent(PSENSOR_STATUS_FAR);
    //register switch dev end

    //create power_en start
    proc_psensor_dir = proc_mkdir("psensor", NULL);
    if (proc_psensor_dir == NULL) {
        printk(KERN_CRIT"%s: create psensor file failed!\n", __func__);
        goto proc_mkdir_failed;
    }
    proc_name = proc_create("power_en", 0666, proc_psensor_dir, &psensor_power_en_fops);
    if (proc_name == NULL) {
        printk(KERN_CRIT"%s: create power_en file failed!\n", __func__);
        goto power_en_create_failed;
    }
    //create power_en end

    ret = iqs128_pinctrl_init(&dev->dev);
    if (ret != 0) {
        printk(KERN_CRIT"%s: iqs128 pinctrl init failed!", __func__);
        goto power_en_create_failed;
    }
    pinctrl_select_state(iqs128_eint_pinctrl, iqs128_eint_active);
    pinctrl_select_state(iqs128_eint_pinctrl, iqs128_power_en_disable);

    //request irq start
    node = of_find_matching_node(node, iqs128_of_match);
    if (node) {
        of_property_read_u32_array(node, "debounce", ints, ARRAY_SIZE(ints));
        iqs128_eint_gpio = ints[0];
        iqs128_eint_debounce = ints[1];
        gpio_set_debounce(iqs128_eint_gpio, iqs128_eint_debounce);
        iqs128_irq = irq_of_parse_and_map(node, 0);
        ret = request_irq(iqs128_irq, (irq_handler_t)iqs128_int_handler,  mt_get_gpio_in(iqs128_eint_gpio | 0x80000000) ? IRQF_TRIGGER_LOW : IRQF_TRIGGER_HIGH, "iqs128", NULL);
        if (ret != 0) {
            printk(KERN_CRIT"%s: psensor request_irq failed!\n", __func__);
            goto power_en_create_failed;
        } else {
            printk(KERN_CRIT"%s: psensor request_irq success iqs128_irq = %d!\n", __func__, iqs128_irq);
        }
        disable_irq(iqs128_irq);
    }
    //request irq end

    //init work start
    iqs128_work = kzalloc(sizeof(typeof(*iqs128_work)), GFP_KERNEL);
    if (!iqs128_work) {
        printk(KERN_CRIT"%s: create work queue error!\n", __func__);
        goto power_en_create_failed;
    }

    INIT_WORK(iqs128_work, do_iqs128_work);
    iqs128_wq = create_singlethread_workqueue("iqs128_wq");
    if (!iqs128_wq) {
        printk(KERN_CRIT"%s: create thread error!\n", __func__);
        goto create_workqueue_failed;
    }
    //init work end

    printk(KERN_CRIT"%s: end!\n", __func__);

    return 0;
create_workqueue_failed:
    kfree(iqs128_work);

power_en_create_failed:
    proc_remove(proc_psensor_dir);

proc_mkdir_failed:
    switch_dev_unregister(iqs128_switch_dev);

switch_dev_register_failed:
    kfree(iqs128_switch_dev);

    printk(KERN_CRIT"%s: end!\n", __func__);
    return -1;
}

static struct platform_driver iqs128_driver = {
    .probe = iqs128_probe,
    .driver = {
        .name = "iqs128_driver",
        .of_match_table = iqs128_of_match,
    },
};

static int __init iqs128_init(void) {
    int ret = 0;

    printk(KERN_CRIT"%s: start!\n", __func__);
    ret = platform_driver_register(&iqs128_driver);
    if (ret)
        printk(KERN_CRIT"%s: platform driver register failed!\n", __func__);
    printk(KERN_CRIT"%s: end!\n", __func__);

    return ret;
}

static void __exit iqs128_exit(void) {
    printk(KERN_CRIT"%s: start!\n", __func__);
    platform_driver_unregister(&iqs128_driver);
    printk(KERN_CRIT"%s: end!\n", __func__);
}

module_init(iqs128_init);
module_exit(iqs128_exit);
MODULE_LICENSE("GPL");
