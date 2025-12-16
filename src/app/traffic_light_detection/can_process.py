def can_process(msg_queue):
    while True:
        msg = msg_queue.get()
        if msg["signal"] == 0:
            print(f"[CAN] NO DETECT CONF={msg['conf']:.2f} INFER={msg['infer_ms']:.1f}ms", flush=True)
        else:
            print(f"[CAN] SIGNAL={msg['signal']} CONF={msg['conf']:.2f} INFER={msg['infer_ms']:.1f}ms", flush=True)