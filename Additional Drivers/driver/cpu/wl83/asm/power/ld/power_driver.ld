		
        *(.power_driver.text)
        *(.power_driver.text.const)
        
        . = ALIGN(4);
        lp_target_begin = .;
        PROVIDE(lp_target_begin = .);
        KEEP(*(.lp_target))
        lp_target_end = .;
        PROVIDE(lp_target_end = .);
        
        . = ALIGN(4);
        lp_request_begin = .;
        PROVIDE(lp_request_begin = .);
        KEEP(*(.lp_request))
        lp_request_end = .;
        PROVIDE(lp_request_end = .);
        
        . = ALIGN(4);
        deepsleep_target_begin = .;
        PROVIDE(deepsleep_target_begin = .);
        KEEP(*(.deepsleep_target))
        deepsleep_target_end = .;
        PROVIDE(deepsleep_target_end = .);
        
        . = ALIGN(4);
        p2m_msg_handler_begin = .;
        PROVIDE(p2m_msg_handler_begin = .);
        KEEP(*(.p2m_msg_handler))
        PROVIDE(p2m_msg_handler_end = .);
        p2m_msg_handler_end = .;
        
        . = ALIGN(4);
        soff_busy_dev_close_hdl_begin = .;
        PROVIDE(soff_busy_dev_close_hdl_begin = .);
        KEEP(*(.soff_busy_dev_close_hdl))
        soff_busy_dev_close_hdl_end = .;
        PROVIDE(soff_busy_dev_close_hdl_end = .);
