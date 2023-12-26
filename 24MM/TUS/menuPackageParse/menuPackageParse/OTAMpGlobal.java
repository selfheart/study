package com.redbend.client.packages;

import java.nio.ByteOrder;
import javolution.io.Struct;

public class OTAMpGlobal extends Struct {
    public  Unsigned8 updateMode = new Unsigned8();
    public  Unsigned8 ecuNumber = new Unsigned8();
    public  Unsigned8 installPauseSocAndApplicability = new Unsigned8();
    public  Unsigned8 installResumeSocAndApplicability = new Unsigned8();
    public  Unsigned8 activitionSocAndApplicability = new Unsigned8();
    public  Unsigned16 vehicleCheckTime = new Unsigned16();
    public  Unsigned16 activationTime = new Unsigned16();

    @Override
    public ByteOrder byteOrder() {
        return ByteOrder.BIG_ENDIAN;
    }

    @Override
    public boolean isPacked() {
        return true;
    }
}