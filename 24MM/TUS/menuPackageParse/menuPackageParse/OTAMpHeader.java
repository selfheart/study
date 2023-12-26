package com.redbend.client.packages;

import java.nio.ByteOrder;
import javolution.io.Struct;

public class OTAMpHeader extends Struct {
    public Unsigned8 pkgTypeVer = new Unsigned8();
    public Unsigned16 headerSize = new Unsigned16();
    public Unsigned32 packetSize = new Unsigned32();
    public UTF8String vin = new UTF8String(17);
    public UTF8String replayProtection = new UTF8String(8);
    public UTF8String hash = new UTF8String(32);

    @Override
    public ByteOrder byteOrder() {
        return ByteOrder.BIG_ENDIAN;
    }

    @Override
    public boolean isPacked() {
        return true;
    }
}