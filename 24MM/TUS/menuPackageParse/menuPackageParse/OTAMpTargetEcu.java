package com.redbend.client.packages;

import java.nio.ByteOrder;
import javolution.io.Struct;

public class OTAMpTargetEcu extends Struct {
    public UTF8String name = new UTF8String(3);
    public Unsigned16 ecuGroup = new Unsigned16();
    public Unsigned16 rebootTime = new Unsigned16();
    public Unsigned16 vehicleCheckTime = new Unsigned16();
    public Unsigned8 waitingTime = new Unsigned8();

    @Override
    public ByteOrder byteOrder() {
        return ByteOrder.BIG_ENDIAN;
    }

    @Override
    public boolean isPacked() {
        return true;
    }
}