package com.redbend.client.common;

import java.io.*;
import java.nio.charset.Charset;
import java.util.Enumeration;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;
import java.util.ArrayList;
import java.util.List;
import java.util.HashMap;
import java.util.Map;
import java.util.Iterator;
import java.nio.ByteBuffer;
import android.content.Context;
import com.redbend.client.common.CommonData;
import com.redbend.client.common.CommonLog;
import com.redbend.client.common.CommonAPI;
import com.redbend.client.packages.OTAMpHeader;
import com.redbend.client.packages.OTAMpGlobal;
import com.redbend.client.packages.OTAMpTargetEcu;
import com.redbend.client.ConnectManager;
import com.redbend.client.InventoryManager;
import com.redbend.client.connect.ConnectPackage;
import iauto.system.ota.IOTAAppServiceListener;

public class OTAMpParser {

    private static final int SIZE_SIGNATRUE = 256;
    private static final int TYPE_MENU = 1;
    private static final int APPLICABLE = 1;
    private static final String ZIPSUFFIX = ".ZIP";
    private static final String FOLDER_MP = "/update/ota/menupackage";
    private ConnectManager mConnectManager = null;
    private ConnectPackage mConnectPackage = null;
    private InputStream mInputStream = null;
    private OutputStream mOutputStream = null;
    private Context mContext = null;
    private byte[] mBytes;
    private String mMpPath;
    private long mMpSize;
    private String mCmsPath;
    private OTAMpHeader mMpHeader = new OTAMpHeader();
    private OTAMpGlobal mMpGlobal = new OTAMpGlobal();
    private OTAMpTargetEcu mMpTargetIVI = new OTAMpTargetEcu();
    private OTAMpTargetEcu mMpTargetIVC = new OTAMpTargetEcu();
    private boolean mInstallPauseApplicability;
    private boolean mInstallResumeApplicability;
    private boolean mActivationApplicability;
    private int mInstallPauseSoc;
    private int mInstallResumeSoc;
    private int mActivationSoc;
    private int mVehicleCheckTime;
    private int mActivationTime;
    private int mIviInstallGroup = 0;
    private int mIviActivitionGroup = 0;
    private int mIvcInstallGroup = CommonData.MP_GROUP_UNDEFINED;
    private int mIvcActivitionGroup = CommonData.MP_GROUP_UNDEFINED;
    private List<String> mEcuList = new ArrayList<String>();
    private int mUpdateObject = 0;
    private boolean mIvcInvolved = false;
    private boolean mGwInvolved = false;
    private boolean mIviInvolved = false;
    private boolean mHasIVIObject = false;
    private boolean mHasIVCObject = false;
    private boolean mHasGWObject = false;
    private int mUpdateMode = -1;

    public OTAMpParser(Context context) {
        mContext = context;
        // set default value
        mInstallPauseSoc = 20;
        mInstallResumeSoc = 25;
    }

    public void resetState() {
        mEcuList.clear();
        mUpdateObject = 0;
        mIvcInvolved = false;
        mGwInvolved = false;
        mIviInvolved = false;
        mHasIVIObject = false;
        mHasIVCObject = false;
        mHasGWObject = false;
    }

    public void setConnectManager(ConnectManager connect) {
        mConnectManager = connect;
    }

    private void close() {
        try {
            if (null != mInputStream) {
                mInputStream.close();
            }
            if (null != mOutputStream) {
                mOutputStream.close();
            }
        } catch (Exception e) {
            CommonLog.writeLogger("e", "close :" , e.getMessage());
        }
    }

    public void setIviInstallGroup(int iviInstallGroup) {
        mIviInstallGroup = iviInstallGroup;
    }

    private boolean isZipFile(String zipPath) {
        if (null == zipPath) {
            return false;
        }
        int lastIndexOf = zipPath.lastIndexOf(".");
        if (-1 == lastIndexOf) {
            return false;
        }
        String suffix = zipPath.substring(lastIndexOf).toUpperCase();
//        mDescDir = zipPath.substring(0, lastIndexOf);
        return ZIPSUFFIX.equals(suffix);
    }

    private  boolean isBinFile(String path) {
        if (null == path) {
            return false;
        }
        int lastIndexOf = path.lastIndexOf(".");
        if (-1 == lastIndexOf) {
            return false;
        }
        String suffix = path.substring(lastIndexOf).toUpperCase();
        String binSuffix = ".BIN";
        return binSuffix.equals(suffix);
    }

    private  boolean isCmsFile(String path) {
        if (null == path) {
            return false;
        }
        int lastIndexOf = path.lastIndexOf(".");
        if (-1 == lastIndexOf) {
            return false;
        }
        String suffix = path.substring(lastIndexOf).toUpperCase();
        String binSuffix = ".CMS";
        return binSuffix.equals(suffix);
    }

    public boolean verifyMP(String zipPath) {
        zipPath = "/update/ota/" + zipPath;
        boolean bRet = unZipFiles(zipPath);
        if (!bRet) {
            return false;
        }

        bRet = verifyMP();
        if (!bRet) {
            return false;
        }

        bRet = parseMpLevel1(true);
        if (!bRet) {
            return false;
        }

        bRet = saveInstallOrder();
        if (!bRet) {
            return false;
        }

        CommonLog.writeLogger("d", "verifyMP ok");
        return true;
    }

    private boolean verifyMP() {
        if (null == mMpPath || null == mCmsPath) {
            CommonLog.writeLogger("d", "mMpPath or mCmsPath is null");
            return false;
        }
        if (null == mConnectManager || !mConnectManager.getReady()) {
            CommonLog.writeLogger("e" , "Connect is NULL, or not Ready");
            return false;
        }
        mConnectPackage = (ConnectPackage) mConnectManager.getConnect(ConnectPackage.class.toString());
        boolean bRet = mConnectPackage.verifyFile(mMpPath, mCmsPath);
        CommonLog.writeLogger("d", "verifyFile: " , bRet);
        return bRet;
    }

    private boolean parseMpLevel1(boolean bCheckVin) {
        if (!getMpHeader()) {
            return false;
        }

        if (!checkMpSize()) {
            return false;
        }

        if (bCheckVin && !checkVIN()) {
            return false;
        }

        if (!getMpGlobal()) {
            return false;
        }

        if (!checkTargetECU()) {
            return false;
        }

        return true;
    }

    private boolean checkMpSize() {
        if (null == mMpHeader) {
            return false;
        }

        int headerSize = Long.valueOf(mMpHeader.headerSize.get()).intValue();
        CommonLog.writeLogger("d", "mMpHeader.headerSize :" , headerSize);

        if (headerSize != mMpHeader.size()) {
            CommonLog.writeLogger("d", "headerSize is error :" , headerSize);
            OTADTLog.DtWrite(OTADTEventDef.DT_EVENT_ID_MP, "headerSize:" + headerSize);
            OTADTLog.DtWrite(OTADTEventDef.DT_EVENT_ID_MP, "mMpHeader.size():" + mMpHeader.size());
            return false;
        }

        int packetSize = Long.valueOf(mMpHeader.packetSize.get()).intValue();
        CommonLog.writeLogger("d", "mMpHeader.packetSize :" , packetSize);

        if (packetSize != mBytes.length) {
            CommonLog.writeLogger("d", "packetSize is error :" , packetSize , ",mBytes length:" , mBytes.length);
            OTADTLog.DtWrite(OTADTEventDef.DT_EVENT_ID_MP, "packetSize:" + packetSize);
            OTADTLog.DtWrite(OTADTEventDef.DT_EVENT_ID_MP, "mBytes.length:" + mBytes.length);
            return false;
        }

        return true;
    }

    private boolean checkVIN() {
        if (null == mMpHeader) {
            return false;
        }
        String vin = mMpHeader.vin.get();
        CommonLog.writeLogger("d", "vin: " , vin);
        String deviceId = InventoryManager.getInstance().GetInventoryData().get("DevId");
        CommonLog.writeLogger("d","deviceId:" , deviceId);
        OTADTLog.DtWrite(OTADTEventDef.DT_EVENT_ID_MP, "deviceId:" + deviceId);

        return deviceId.equals(vin);
    }

    private boolean getMpHeader() {
        if (null == mMpHeader) {
            return false;
        }

        byte[] mpHeaderBytes = new byte[mMpHeader.size()];

        try {
            System.arraycopy(mBytes, 0, mpHeaderBytes, 0, mpHeaderBytes.length);
            ByteBuffer byteBuffer = ByteBuffer.wrap(mpHeaderBytes);
            byteBuffer.order(mMpHeader.byteOrder());
            mMpHeader.setByteBuffer(byteBuffer, 0);
        } catch (Exception e) {
            CommonLog.writeLogger("e", "Failed Exception: " , e.getMessage());
            OTADTLog.DtWrite(OTADTEventDef.DT_EVENT_ID_MP, "getMpHeader:" + e.getMessage());
            return false;
        }
        int pkgTypeVer = Long.valueOf(mMpHeader.pkgTypeVer.get()).intValue();
        int pkgType = (pkgTypeVer & 0xf0) >> 4;
        CommonLog.writeLogger("d", "pkgTypeVer: " , pkgTypeVer , ", pkgType: " , pkgType);

        if (TYPE_MENU != pkgType) {
            OTADTLog.DtWrite(OTADTEventDef.DT_EVENT_ID_MP, "pkgType:" + pkgType);
            return false;
        }

        return true;
    }

    private boolean getMpGlobal() {
        if (null == mMpGlobal) {
            return false;
        }

        byte[] mpGlobalBytes = new byte[mMpGlobal.size()];

        try {
            System.arraycopy(mBytes, mMpHeader.size() + SIZE_SIGNATRUE, mpGlobalBytes, 0, mpGlobalBytes.length);
            ByteBuffer byteBuffer = ByteBuffer.wrap(mpGlobalBytes);
            byteBuffer.order(mMpGlobal.byteOrder());
            mMpGlobal.setByteBuffer(byteBuffer, 0);
        } catch (Exception e) {
            CommonLog.writeLogger("e", "Failed Exception: " , e.getMessage());
            OTADTLog.DtWrite(OTADTEventDef.DT_EVENT_ID_MP, "getMpGlobal:" + e.getMessage());
            return false;
        }

        parseSoc();

        return true;
    }

    private boolean parseSoc() {
        if (null == mMpGlobal) {
            return false;
        }

        int updateMode = Long.valueOf(mMpGlobal.updateMode.get()).intValue();
        int installPauseSocAndApplicability = Long.valueOf(mMpGlobal.installPauseSocAndApplicability.get()).intValue();
        int installResumeSocAndApplicability = Long.valueOf(mMpGlobal.installResumeSocAndApplicability.get()).intValue();
        int activitionSocAndApplicability = Long.valueOf(mMpGlobal.activitionSocAndApplicability.get()).intValue();
        int vehicleCheckTime = Long.valueOf(mMpGlobal.vehicleCheckTime.get()).intValue();
        int activationTime = Long.valueOf(mMpGlobal.activationTime.get()).intValue();

        mInstallPauseApplicability = ((installPauseSocAndApplicability & 0x80) >> 7) == APPLICABLE;
        mInstallResumeApplicability = ((installResumeSocAndApplicability & 0x80) >> 7) == APPLICABLE;
        mActivationApplicability = ((activitionSocAndApplicability & 0x80) >> 7) == APPLICABLE;
        mInstallPauseSoc = installPauseSocAndApplicability & 0x7f;
        mInstallResumeSoc = installResumeSocAndApplicability & 0x7f;
        mActivationSoc = activitionSocAndApplicability & 0x7f;
        mVehicleCheckTime = vehicleCheckTime & 0xffff;
        mActivationTime = activationTime& 0xffff;
        mUpdateMode = updateMode;

        CommonLog.writeLogger("d", "mInstallPauseApplicability: " , mInstallPauseApplicability);
        CommonLog.writeLogger("d", "mInstallResumeApplicability: " , mInstallResumeApplicability);
        CommonLog.writeLogger("d", "mActivationApplicability: " , mActivationApplicability);
        CommonLog.writeLogger("d", "mInstallPauseSoc: " , mInstallPauseSoc);
        CommonLog.writeLogger("d", "mInstallResumeSoc: " , mInstallResumeSoc);
        CommonLog.writeLogger("d", "mActivationSoc: " , mActivationSoc);
        CommonLog.writeLogger("d", "mVehicleCheckTime: " , mVehicleCheckTime);
        CommonLog.writeLogger("d", "mActivationTime: " , mActivationTime);
        CommonLog.writeLogger("d", "mUpdateMode: " , mUpdateMode);
        OTADTLog.DtWrite(OTADTEventDef.DT_EVENT_ID_MP, "mInstallPauseApplicability:" + mInstallPauseApplicability);
        OTADTLog.DtWrite(OTADTEventDef.DT_EVENT_ID_MP, "mInstallResumeApplicability:" + mInstallResumeApplicability);
        OTADTLog.DtWrite(OTADTEventDef.DT_EVENT_ID_MP, "mActivationApplicability:" + mActivationApplicability);
        OTADTLog.DtWrite(OTADTEventDef.DT_EVENT_ID_MP, "mInstallPauseSoc:" + mInstallPauseSoc);
        OTADTLog.DtWrite(OTADTEventDef.DT_EVENT_ID_MP, "mInstallResumeSoc:" + mInstallResumeSoc);
        OTADTLog.DtWrite(OTADTEventDef.DT_EVENT_ID_MP, "mActivationSoc:" + mActivationSoc);
        OTADTLog.DtWrite(OTADTEventDef.DT_EVENT_ID_MP, "mVehicleCheckTime:" + mVehicleCheckTime);
        OTADTLog.DtWrite(OTADTEventDef.DT_EVENT_ID_MP, "mActivationTime:" + mActivationTime);
        OTADTLog.DtWrite(OTADTEventDef.DT_EVENT_ID_MP, "mUpdateMode:" + mUpdateMode);
        // cy2020+ valid update mode is 0 or 1
        if (mUpdateMode != 0 && mUpdateMode != 1) {
            return false;
        }
        return true;
    }

    private boolean checkTargetECU() {
        if ((null == mMpHeader) || (null == mMpGlobal) || (null == mBytes)) {
            return false;
        }

        int targetEcuSizeReal = mBytes.length - mMpHeader.size() - mMpGlobal.size() - SIZE_SIGNATRUE;
        int ecuNum = Long.valueOf(mMpGlobal.ecuNumber.get()).intValue();
        int targetEcuSize = mMpTargetIVI.size() * ecuNum;
        CommonLog.writeLogger("d", "targetEcuSizeReal : " , targetEcuSizeReal , ", targetEcuSize : " , targetEcuSize);
        if (targetEcuSizeReal != targetEcuSize) {
            OTADTLog.DtWrite(OTADTEventDef.DT_EVENT_ID_MP, "targetEcuSizeReal:" + targetEcuSizeReal);
            OTADTLog.DtWrite(OTADTEventDef.DT_EVENT_ID_MP, "targetEcuSize:" + targetEcuSize);
            return false;
        }

        int socPos = mMpHeader.size() + mMpGlobal.size() + SIZE_SIGNATRUE;
        mEcuList.clear();
        while (socPos != mBytes.length) {
            OTAMpTargetEcu mpTargetEcu = new OTAMpTargetEcu();
            byte[] mpTargetEcuByte = new byte[mpTargetEcu.size()];

            try {
                System.arraycopy(mBytes, socPos, mpTargetEcuByte, 0, mpTargetEcuByte.length);
                socPos += mpTargetEcuByte.length;
                ByteBuffer byteBuffer = ByteBuffer.wrap(mpTargetEcuByte);
                byteBuffer.order(mpTargetEcu.byteOrder());
                mpTargetEcu.setByteBuffer(byteBuffer, 0);
            } catch (Exception e) {
                CommonLog.writeLogger("e", "Failed Exception: ", e.getMessage());
                OTADTLog.DtWrite(OTADTEventDef.DT_EVENT_ID_MP, "checkTargetECU:" + e.getMessage());
                return false;
            }
            CommonLog.writeLogger("d", "TargetEcu.name: ", mpTargetEcu.name.get());
            OTADTLog.DtWrite(OTADTEventDef.DT_EVENT_ID_MP, "TargetEcu:" + mpTargetEcu.name.get());

            if (CommonData.ECU_TYPE_RDO.equals(mpTargetEcu.name.get())) {
                mMpTargetIVI = mpTargetEcu;
                parseIVIInfo();
                mEcuList.add(mpTargetEcu.name.get());
                mUpdateObject |= IOTAAppServiceListener.SOC;
            } else if (CommonData.ECU_TYPE_TCU.equals(mpTargetEcu.name.get())) {
                mMpTargetIVC = mpTargetEcu;
                parseIVCInfo();
                mEcuList.add(mpTargetEcu.name.get());
                mUpdateObject |= IOTAAppServiceListener.TCU_OBJ;
            } else {
                OTACarDataMonitor otaCarDataMonitor = new OTACarDataMonitor(mContext);
                if (!addEcuType(mpTargetEcu.name.get(), otaCarDataMonitor.isBlockGW())) {
                    return false;
                }
            }
        }
        OTACarDataMonitor otaCarDataMonitor = new OTACarDataMonitor(mContext);
        if (otaCarDataMonitor.isBlockGW()) {
            if ((mUpdateObject & IOTAAppServiceListener.SOC) == IOTAAppServiceListener.SOC
                && (mUpdateObject & IOTAAppServiceListener.TCU_OBJ) == IOTAAppServiceListener.TCU_OBJ) {
                return false;
            }
        }

        return true;
    }

    private boolean addEcuType(String ecuName, boolean isBlockGW) {
        CommonLog.writeLogger("d", "ecuName: ", ecuName , ", isBlockGW: " , isBlockGW);
        if (!isBlockGW) {
            mEcuList.add(ecuName);
            mUpdateObject |= IOTAAppServiceListener.TCU_OBJ;
            return true;
        } else {
            return false;
        }
    }

    private void parseIVIInfo() {
        if (null == mMpTargetIVI) {
            CommonLog.writeLogger("d", "null == mMpTargetIVI");
            return;
        }
        int ecuGroup = Long.valueOf(mMpTargetIVI.ecuGroup.get()).intValue();
        mIviInstallGroup = (ecuGroup & 0x3e0) >> 5;
        mIviActivitionGroup = ecuGroup & 0x1f;
        CommonLog.writeLogger("d", "mIviInstallGroup: " , mIviInstallGroup);
        CommonLog.writeLogger("d", "mIviActivitionGroup: " , mIviActivitionGroup);
    }

    private void parseIVCInfo() {
        if (null == mMpTargetIVC) {
            CommonLog.writeLogger("d", "null == mMpTargetIVC");
            return;
        }
        int ecuGroup = Long.valueOf(mMpTargetIVC.ecuGroup.get()).intValue();
        mIvcInstallGroup = (ecuGroup & 0x3e0) >> 5;
        mIvcActivitionGroup = ecuGroup & 0x1f;
        CommonLog.writeLogger("d", "mIvcInstallGroup: " , mIvcInstallGroup);
        CommonLog.writeLogger("d", "mIvcActivitionGroup: " , mIvcActivitionGroup);
    }

    private boolean unZipFiles(String zipPath) {
//        if (!isZipFile(zipPath)) {
//            return false;
//        }
        CommonLog.writeLogger("d", "zipPath: " , zipPath);

        boolean bMkdir = CommonAPI.createFolder(FOLDER_MP, 0x770, "vendor_ota");
        if(false == bMkdir) {
            CommonLog.writeLogger("e" , "create /update/ota/menupackage failed");
            return false;
        }

        ZipFile zip = null;
        try {
            File zipFile = new File(zipPath);
            File pathFile = new File(FOLDER_MP);
            if (!pathFile.exists()) {
                pathFile.mkdirs();
            }

            zip = new ZipFile(zipFile, Charset.forName("GBK"));
            for (Enumeration entries = zip.entries(); entries.hasMoreElements(); ) {
                ZipEntry entry = (ZipEntry) entries.nextElement();
                String zipEntryName = entry.getName();
                mInputStream = zip.getInputStream(entry);
                String outPath = (FOLDER_MP + "/" + zipEntryName).replaceAll("\\*", "/");

                File file = new File(outPath.substring(0, outPath.lastIndexOf('/')));
                if (!file.exists()) {
                    file.mkdirs();
                }

                if (new File(outPath).isDirectory()) {
                    continue;
                }
                CommonAPI.changeFileAttr(outPath, 0x660, "vendor_ota");
                CommonLog.writeLogger("d", "outPath = ", outPath);
                mOutputStream = new FileOutputStream(outPath);
                byte[] data = new byte[1024];
                int len = 0;
                ByteArrayOutputStream byteArray = new ByteArrayOutputStream();
                DataOutputStream dataOutputStream = new DataOutputStream(byteArray);

                while ((len = mInputStream.read(data)) > 0) {
                    mOutputStream.write(data, 0, len);
                    dataOutputStream.write(data, 0, len);
                }
                if (isBinFile(outPath)) {
                    mBytes = byteArray.toByteArray();
                    mMpPath = outPath;
                    mMpSize = mBytes.length;
                } else if (isCmsFile(outPath)) {
                    mCmsPath = outPath;
                }
            }
        } catch (Exception e) {
            CommonLog.writeLogger("e", "unZipFiles :" , e.getMessage());
            return false;
        } finally {
            try {
                if (zip != null) {
                    zip.close();
                }
                close();
            } catch (Exception e) {
                CommonLog.writeLogger("e", "Exception:" , e.getMessage());
            }
        }
        return true;
    }

    private boolean saveInstallOrder() {
        OutputStream out = null;
        try {
            CommonLog.writeLogger("d", "start saveInstallOrder");
            out = new FileOutputStream("/data/user_de/0/com.redbend.client/files/installation_order.txt");
            StringBuilder sb = new StringBuilder();
            for (String ecuName : mEcuList) {
                addInstallOrder(sb, ecuName);
            }
            out.write(sb.toString().getBytes(), 0, sb.toString().length());
            out.flush();
            CommonLog.writeLogger("d", "end save file");
        } catch (Exception e) {
            CommonLog.writeLogger("e" , "write file failed" , e.toString());
            OTADTLog.DtWrite(OTADTEventDef.DT_EVENT_ID_INVENTORYMANAGER_SAVEIVIHW_FAIL);
            return false;
        } finally {
            if (out != null) {
                try {
                    out.close();
                } catch (Exception e) {
                    CommonLog.writeLogger("e", "Exception:" , e.getMessage());
                }
            }
        }
        return true;
    }

    private void addInstallOrder(StringBuilder sb, String ecuName) {
        if (null == sb) {
            CommonLog.writeLogger("d" , "StringBuilder is null");
            return;
        }
        if (CommonData.mapInstallType.containsKey(ecuName)) {
            sb.append("GROUP_PARALLEL_BEGIN\n");
            sb.append("INSTALLER=");
            sb.append(CommonData.mapInstallType.get(ecuName));
            sb.append("\n");
            sb.append("GROUP_PARALLEL_END\n");
        } else {
            CommonLog.writeLogger("d" , "ecuName error: " , ecuName);
        }
    }

    public boolean getActivationApplicability() {
        CommonLog.writeLogger("d" , "mActivationApplicability: " , mActivationApplicability);
        return mActivationApplicability;
    }

    public boolean getInsApplicability() {
        CommonLog.writeLogger("d" , "mInstallResumeApplicability: " , mInstallResumeApplicability);
        return mInstallResumeApplicability;
    }

    public int getActivationSoc() {
        CommonLog.writeLogger("d" , "mActivationSoc: " , mActivationSoc);
        return mActivationSoc;
    }

    public int getVehicleCheckTime() {
        CommonLog.writeLogger("d" , "mVehicleCheckTime: " , mVehicleCheckTime);
        return mVehicleCheckTime;
    }

    public int getActivationTime() {
        CommonLog.writeLogger("d" , "mActivationTime: " , mActivationTime);
        return mActivationTime;
    }

    public int getUpdateMode() {
        CommonLog.writeLogger("d" , "mUpdateMode: " , mUpdateMode);
        return mUpdateMode;
    }

    public boolean getInstallPauseApplicability() {
        CommonLog.writeLogger("d" , "mInstallPauseApplicability: " , mInstallPauseApplicability);
        return mInstallPauseApplicability;
    }

    public boolean getInstallResumeApplicability() {
        CommonLog.writeLogger("d" , "mInstallResumeApplicability: " , mInstallResumeApplicability);
        return mInstallResumeApplicability;
    }

    public int getInstallPauseSoc() {
        CommonLog.writeLogger("d" , "mInstallPauseSoc: " , mInstallPauseSoc);
        return mInstallPauseSoc;
    }

    public int getInstallResumeSoc() {
        CommonLog.writeLogger("d" , "mInstallResumeSoc: " , mInstallResumeSoc);
        return mInstallResumeSoc;
    }

    public int getIVCInstallGroup() {
        CommonLog.writeLogger("d" , "mIvcInstallGroup: " , mIvcInstallGroup);
        return mIvcInstallGroup;
    }

    public int getIVCActivationGroup() {
        CommonLog.writeLogger("d" , "mIvcActivitionGroup: " , mIvcActivitionGroup);
        return mIvcActivitionGroup;
    }

    public int getIVIInstallGroup() {
        CommonLog.writeLogger("d" , "mIviInstallGroup: " , mIviInstallGroup);
        return mIviInstallGroup;
    }

    public int getIVIActivationGroup() {
        CommonLog.writeLogger("d" , "mIviActivitionGroup: " , mIviActivitionGroup);
        return mIviActivitionGroup;
    }

    public int getFixedTargetNum(int curObj) {
        CommonLog.writeLogger("d" , "getFixedTargetNum curObj:", curObj);
        int fixedNum = 0;
        String strCurObj = CommonData.TransNumToInstallType(curObj);
        int curIndex = mEcuList.indexOf(strCurObj);
        boolean needFixMeter = false;
        boolean needFixHud = false;
        if (curIndex > 0) {
            for (int i = 0; i < curIndex; ++i) {
                if (mEcuList.get(i).equals(CommonData.ECU_TYPE_TDB)) {     // meter , only add once
                    needFixMeter = true;
                } else if (mEcuList.get(i).equals(CommonData.ECU_TYPE_HMD)) {   // hud , only add once
                    needFixHud = true;
                }
            }
        }
        if (needFixMeter) {
            ++fixedNum;
        }
        if (needFixHud) {
            ++fixedNum;
        }
        return fixedNum;
    }

    public int getTargetEcuNum() {
        int num = mEcuList.size();
        if (mEcuList.contains(CommonData.ECU_TYPE_TDB)) {
            CommonLog.writeLogger("d" , "ecuList has TDB");
            ++num;
        }
        if (mEcuList.contains(CommonData.ECU_TYPE_HMD)) {
            CommonLog.writeLogger("d" , "ecuList has HMD");
            ++num;
        }
        CommonLog.writeLogger("d" , "num: " , num);
        return num;
    }

    public int getTargetEcuIdx(String typeNum) {
        int idx = 0;
        String typeName = null;
        Iterator it = CommonData.mapInstallType.entrySet().iterator();
        while (it.hasNext()) {
            Map.Entry entry = (Map.Entry)it.next();
            if (entry.getValue().equals(typeNum)) {
                typeName = (String)entry.getKey();
                break;
            }
        }
        CommonLog.writeLogger("d", "getTargetEcuIdx typeName: ", typeName, " list: ", mEcuList);
        if (typeName != null) {
            idx = mEcuList.indexOf(typeName);
            ++idx;
        }
        CommonLog.writeLogger("d" , "idx:" , idx);
        return idx;
    }

    public int getUpdateObject() {
        CommonLog.writeLogger("d" , "mUpdateObject: " , mUpdateObject);
        return mUpdateObject;
    }

    public long getMpSize() {
        CommonLog.writeLogger("d" , "getMpSize: " , mMpSize);
        return mMpSize;
    }

    public String getMpPath() {
        CommonLog.writeLogger("d" , "mMpPath: " , mMpPath);
        return mMpPath;
    }

    public void setMpPath(String mpPath) {
        CommonLog.writeLogger("d" , "mpPath: " , mpPath);
        mMpPath = mpPath;
    }

    public String getCmsPath() {
        CommonLog.writeLogger("d" , "mCmsPath: " , mCmsPath);
        return mCmsPath;
    }

    public void setCmsPath(String cmsPath) {
        CommonLog.writeLogger("d" , "cmsPath: " , cmsPath);
        mCmsPath = cmsPath;
    }

    public void parseMP() {
        CommonLog.writeLogger("d" , "parseMP");

        boolean bRet = loadFile(mMpPath);
        if (!bRet) {
            CommonLog.writeLogger("d" , "loadFile return false");
            return;
        }
        CommonLog.writeLogger("d" , "mBytes.length: " , mBytes.length);

        bRet = parseMpLevel1(false);
        if (!bRet) {
            CommonLog.writeLogger("d" , "parseMpLevel1 return false");
            return;
        }

        bRet = saveInstallOrder();
        if (!bRet) {
            CommonLog.writeLogger("d" , "saveInstallOrder return false");
            return;
        }
    }

    private boolean loadFile(String filePath) {
        boolean bRet = false;
        File file = new File(filePath);
        mMpSize = file.length();
        InputStream inputStream = null;
        CommonLog.writeLogger("d" , "filePath:", filePath);
        CommonLog.writeLogger("d" , "file size:", file.length());
        try {
            inputStream = new FileInputStream(file);
            ByteArrayOutputStream byteArray = new ByteArrayOutputStream();
            DataOutputStream dataOutputStream = new DataOutputStream(byteArray);
            int len;
            byte[] data = new byte[1024];
            while ((len = inputStream.read(data)) > 0) {
                dataOutputStream.write(data, 0, len);
            }
            if (null != mInputStream) {
                mInputStream.close();
            }
            if (null != dataOutputStream) {
                dataOutputStream.close();
            }

            mBytes = byteArray.toByteArray();
            bRet = true;
        } catch (Exception e) {
            CommonLog.writeLogger("e" , "Exception:" , e.toString());
        } finally {
            if (inputStream != null) {
                try {
                    inputStream.close();
                } catch (Exception e) {
                    CommonLog.writeLogger("e" , "Exception:" , e.toString());
                }
            }
        }
        return bRet;
    }

    public void parseCompletedObject(int curObj) {
        CommonLog.writeLogger("d" , "curObj:", curObj);
        String strCurObj = CommonData.TransNumToInstallType(curObj);
        int index = mEcuList.indexOf(strCurObj);
        if (index < 0) {
            CommonLog.writeLogger("e" , "curObj is not in mEcuList:");
            return;
        }
        for (int i = 0; i <= index; ++i) {
            if (mEcuList.get(i).equals(CommonData.ECU_TYPE_TCU)) {
                mIvcInvolved = true;
            } else if (mEcuList.get(i).equals(CommonData.ECU_TYPE_RDO)) {
                mIviInvolved = true;
            } else {
                mGwInvolved = true;
            }
        }
    }

    public boolean isIvcInvolved() {
        CommonLog.writeLogger("d" , "mIvcInvolved:", mIvcInvolved);
        return mIvcInvolved;
    }

    public boolean isGwInvolved() {
        CommonLog.writeLogger("d" , "mGwInvolved:", mGwInvolved);
        return mGwInvolved;
    }

    public boolean isIviInvolved() {
        CommonLog.writeLogger("d" , "mIviInvolved:", mIviInvolved);
        return mIviInvolved;
    }

    public void parseObjectListContent() {
        CommonLog.writeLogger("d" , "parseObjectListContent");
        for (int i = 0; i < mEcuList.size(); ++i) {
            if (mEcuList.get(i).equals(CommonData.ECU_TYPE_TCU)) {
                mHasIVCObject = true;
            } else if (mEcuList.get(i).equals(CommonData.ECU_TYPE_RDO)) {
                mHasIVIObject = true;
            } else {
                mHasGWObject = true;
            }
        }
        CommonLog.writeLogger("d" , "parseObjectListContent end , hasIVI : ", mHasIVIObject, " hasIVC : ", mHasIVCObject, " hasGw : ", mHasGWObject);
    }

    public boolean containsIVIObject() {
        return mHasIVIObject;
    }

    public boolean containsIVCObject() {
        return mHasIVCObject;
    }

    public boolean containsGWObject() {
        return mHasGWObject;
    }

    public int getObjectByIdx(int idx) {
        String idxObject = "";
        if (idx < mEcuList.size()) {
            idxObject = mEcuList.get(idx);
        }
        CommonLog.writeLogger("d" , "idx: ", idx, " object: ", idxObject);
        int transIdxObject = -1;
        if (CommonData.mapInstallType.containsKey(idxObject)) {
            transIdxObject = Integer.parseInt(CommonData.mapInstallType.get(idxObject));
        }
        return transIdxObject;
    }

    public boolean isLastObject(int curObj) {
        CommonLog.writeLogger("d" , "curObj:", curObj);
        String strCurObj = CommonData.TransNumToInstallType(curObj);
        int index = mEcuList.indexOf(strCurObj);
        CommonLog.writeLogger("d" , "index:", index);
        return index == mEcuList.size() - 1;
    }

    public List<String> getTotalObject() {
        return mEcuList;
    }
}
