## root-cell配置
```
struct {
	struct jailhouse_system header;
	__u64 cpus[1];
	struct jailhouse_memory mem_regions[4];
	struct jailhouse_irqchip irqchips[1];
	struct jailhouse_pci_device pci_devices[1];
}
```

#### struct jailhouse_system
```
struct jailhouse_system {
	char signature[6];
	__u16 revision;
	__u32 flags;

	/** Jailhouse's location in memory */
	struct jailhouse_memory hypervisor_memory;
	struct jailhouse_console debug_console;
	struct {
		__u64 pci_mmconfig_base;
		__u8 pci_mmconfig_end_bus;
		__u8 pci_is_virtual;
		__u16 pci_domain;
		union {
			struct {
				__u16 pm_timer_address;
				__u32 vtd_interrupt_limit;
				__u8 apic_mode;
				__u8 padding[3];
				__u32 tsc_khz;
				__u32 apic_khz;
				struct jailhouse_iommu
					iommu_units[JAILHOUSE_MAX_IOMMU_UNITS];
			} __attribute__((packed)) x86;
			struct {
				u8 maintenance_irq;
				u8 gic_version;
				u64 gicd_base;
				u64 gicc_base;
				u64 gich_base;
				u64 gicv_base;
				u64 gicr_base;
			} __attribute__((packed)) arm;
		} __attribute__((packed));
	} __attribute__((packed)) platform_info;
	struct jailhouse_cell_desc root_cell;
} __attribute__((packed));
```

- .signature: 可取值JAILHOUSE_SYSTEM_SIGNATURE、JAILHOUSE_CELL_DESC_SIGNATURE
- .revision: 一般取JAILHOUSE_CONFIG_REVISION，jailhouse_cmd_enable和jailhouse_cmd_cell_create做检查。
- .flags
  - JAILHOUSE_SYS_VIRTUAL_DEBUG_CONSOLE: 允许root cell从虚拟控制台读取
  - JAILHOUSE_CELL_VIRTUAL_CONSOLE_PERMITTED: 允许non-root cell调用dbg_putc hypercall写hypervisor的控制台，用于debug，root cell据此可以读inmate的输出
  - JAILHOUSE_CELL_VIRTUAL_CONSOLE_ACTIVE: If JAILHOUSE_CELL_VIRTUAL_CONSOLE_ACTIVE is set, inmates should use the virtual console. This flag implies JAILHOUSE_CELL_VIRTUAL_CONSOLE_PERMITTED.
- .hypervisor_memory: 
  - .phys_start: 物理起始地址，根据不同板子设置不同的地址
  - .size: 为jailhouse预留的内存大小，根据不同的板子一般取4MB和64MB
  - .virt_start
  - .flags
- debug_console:
  - .address
  - .size
  - .type: 根据需要选择不同的类型，类型定义于include/jailhouse/console.h
  - .flags: 同见include/jailhouse/console.h
 ```
struct jailhouse_console {
	__u64 address;
	__u32 size;
	__u16 type;
	__u16 flags;
	__u32 divider;
	__u32 gate_nr;
	__u64 clock_reg;
} __attribute__((packed));
```
- .platform_info:
- .root_cell: 
```
struct jailhouse_cell_desc {
	char signature[6];
	__u16 revision;

	char name[JAILHOUSE_CELL_NAME_MAXLEN+1];
	__u32 id; /* set by the driver */
	__u32 flags;

	__u32 cpu_set_size;
	__u32 num_memory_regions;
	__u32 num_cache_regions;
	__u32 num_irqchips;
	__u32 num_pio_regions;
	__u32 num_pci_devices;
	__u32 num_pci_caps;

	__u32 vpci_irq_base;

	__u64 cpu_reset_address;
	__u64 msg_reply_timeout;

	struct jailhouse_console console;
} __attribute__((packed));
```
- .irqchips: 中断处理配置，x86对应IOAPIC, arm对应gic配置
```
struct jailhouse_irqchip {
	__u64 address;
	__u32 id;
	__u32 pin_base;
	__u32 pin_bitmap[4];
} __attribute__((packed));
```

```
if (config->revision != JAILHOUSE_CONFIG_REVISION) {
		pr_err("jailhouse: Configuration revision mismatch\n");
		err = -EINVAL;
		goto kfree_config_out;
	}
```
