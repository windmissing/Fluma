#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
namespace grs::g9spf {
inline constexpr const char* kEngineeringSha="0ae205834bbf0efde09768ec48551925570effbc3b4b5f2d1ad2ed467e203a82";
inline constexpr uint32_t kProfileCode=826691667u,kSeal=3235776769u;
inline constexpr uint32_t kStateMagic=827537735u;
inline constexpr uint32_t kEventMagic=826620231u;
inline constexpr uint32_t kPathMagic=827341127u;
inline constexpr uint32_t kInputMagic=826882375u;
inline constexpr uint32_t kCompletionMagic=826489159u;
namespace header {
inline constexpr size_t magic=0;
inline constexpr size_t revision=1;
inline constexpr size_t profile=2;
inline constexpr size_t flags=3;
inline constexpr size_t runLo=4;
inline constexpr size_t runHi=5;
inline constexpr size_t ownerLo=6;
inline constexpr size_t ownerHi=7;
inline constexpr size_t contextLo=8;
inline constexpr size_t contextHi=9;
inline constexpr size_t commitLo=10;
inline constexpr size_t commitHi=11;
inline constexpr size_t acceptedQuantaLo=12;
inline constexpr size_t acceptedQuantaHi=13;
inline constexpr size_t inputGenerationLo=14;
inline constexpr size_t inputGenerationHi=15;
inline constexpr size_t lastNonceLo=16;
inline constexpr size_t lastNonceHi=17;
inline constexpr size_t nextId=18;
inline constexpr size_t originCount=19;
inline constexpr size_t angleIndex=20;
inline constexpr size_t gQ20=21;
inline constexpr size_t gxQ20=22;
inline constexpr size_t gnQ20=23;
inline constexpr size_t K=24;
inline constexpr size_t N=25;
inline constexpr size_t Ls=26;
inline constexpr size_t Lf=27;
inline constexpr size_t lastSourceSeqLo=28;
inline constexpr size_t lastSourceSeqHi=29;
inline constexpr size_t lastTouchSeqLo=30;
inline constexpr size_t lastTouchSeqHi=31;
inline constexpr size_t lastSourceCommitLo=32;
inline constexpr size_t lastSourceCommitHi=33;
inline constexpr size_t lastTouchCommitLo=34;
inline constexpr size_t lastTouchCommitHi=35;
inline constexpr size_t lastSourceOutcome=36;
inline constexpr size_t lastTouchOutcome=37;
inline constexpr size_t lastSourceAdmittedLo=38;
inline constexpr size_t lastSourceAdmittedHi=39;
inline constexpr size_t totalMassLo=40;
inline constexpr size_t totalMassHi=41;
inline constexpr size_t parameterSha0=42;
inline constexpr size_t parameterSha1=43;
inline constexpr size_t parameterSha2=44;
inline constexpr size_t parameterSha3=45;
inline constexpr size_t parameterSha4=46;
inline constexpr size_t parameterSha5=47;
inline constexpr size_t parameterSha6=48;
inline constexpr size_t parameterSha7=49;
inline constexpr size_t cumulativeGravityJx0=50;
inline constexpr size_t cumulativeGravityJx1=51;
inline constexpr size_t cumulativeGravityJx2=52;
inline constexpr size_t cumulativeGravityJx3=53;
inline constexpr size_t cumulativeGravityJy0=54;
inline constexpr size_t cumulativeGravityJy1=55;
inline constexpr size_t cumulativeGravityJy2=56;
inline constexpr size_t cumulativeGravityJy3=57;
inline constexpr size_t cumulativeTouchJx0=58;
inline constexpr size_t cumulativeTouchJx1=59;
inline constexpr size_t cumulativeTouchJx2=60;
inline constexpr size_t cumulativeTouchJx3=61;
inline constexpr size_t cumulativeTouchJy0=62;
inline constexpr size_t cumulativeTouchJy1=63;
inline constexpr size_t cumulativeTouchJy2=64;
inline constexpr size_t cumulativeTouchJy3=65;
inline constexpr size_t cumulativeSourceJx0=66;
inline constexpr size_t cumulativeSourceJx1=67;
inline constexpr size_t cumulativeSourceJx2=68;
inline constexpr size_t cumulativeSourceJx3=69;
inline constexpr size_t cumulativeSourceJy0=70;
inline constexpr size_t cumulativeSourceJy1=71;
inline constexpr size_t cumulativeSourceJy2=72;
inline constexpr size_t cumulativeSourceJy3=73;
inline constexpr size_t cumulativeSupportJx0=74;
inline constexpr size_t cumulativeSupportJx1=75;
inline constexpr size_t cumulativeSupportJx2=76;
inline constexpr size_t cumulativeSupportJx3=77;
inline constexpr size_t cumulativeSupportJy0=78;
inline constexpr size_t cumulativeSupportJy1=79;
inline constexpr size_t cumulativeSupportJy2=80;
inline constexpr size_t cumulativeSupportJy3=81;
inline constexpr size_t cumulativeGravityWork0=82;
inline constexpr size_t cumulativeGravityWork1=83;
inline constexpr size_t cumulativeGravityWork2=84;
inline constexpr size_t cumulativeGravityWork3=85;
inline constexpr size_t cumulativeTouchWork0=86;
inline constexpr size_t cumulativeTouchWork1=87;
inline constexpr size_t cumulativeTouchWork2=88;
inline constexpr size_t cumulativeTouchWork3=89;
inline constexpr size_t cumulativeSourceWork0=90;
inline constexpr size_t cumulativeSourceWork1=91;
inline constexpr size_t cumulativeSourceWork2=92;
inline constexpr size_t cumulativeSourceWork3=93;
inline constexpr size_t cumulativeDissipationLower0=94;
inline constexpr size_t cumulativeDissipationLower1=95;
inline constexpr size_t cumulativeDissipationLower2=96;
inline constexpr size_t cumulativeDissipationLower3=97;
inline constexpr size_t muStartQ16=98;
inline constexpr size_t muStopQ16=99;
inline constexpr size_t bandQ20=100;
inline constexpr size_t entrainRateQ16=101;
inline constexpr size_t shearCoeffQ16=102;
inline constexpr size_t settleTicks=103;
inline constexpr size_t lastSourceSeqValid=104;
inline constexpr size_t lastTouchSeqValid=105;
inline constexpr size_t reserved_106=106;
inline constexpr size_t reserved_107=107;
inline constexpr size_t reserved_108=108;
inline constexpr size_t reserved_109=109;
inline constexpr size_t reserved_110=110;
inline constexpr size_t reserved_111=111;
inline constexpr size_t reserved_112=112;
inline constexpr size_t reserved_113=113;
inline constexpr size_t reserved_114=114;
inline constexpr size_t reserved_115=115;
inline constexpr size_t reserved_116=116;
inline constexpr size_t reserved_117=117;
inline constexpr size_t reserved_118=118;
inline constexpr size_t reserved_119=119;
inline constexpr size_t reserved_120=120;
inline constexpr size_t reserved_121=121;
inline constexpr size_t reserved_122=122;
inline constexpr size_t reserved_123=123;
inline constexpr size_t reserved_124=124;
inline constexpr size_t reserved_125=125;
inline constexpr size_t reserved_126=126;
inline constexpr size_t reserved_127=127;
inline constexpr size_t reserved_128=128;
inline constexpr size_t reserved_129=129;
inline constexpr size_t reserved_130=130;
inline constexpr size_t reserved_131=131;
inline constexpr size_t reserved_132=132;
inline constexpr size_t reserved_133=133;
inline constexpr size_t reserved_134=134;
inline constexpr size_t reserved_135=135;
inline constexpr size_t reserved_136=136;
inline constexpr size_t reserved_137=137;
inline constexpr size_t reserved_138=138;
inline constexpr size_t reserved_139=139;
inline constexpr size_t reserved_140=140;
inline constexpr size_t reserved_141=141;
inline constexpr size_t reserved_142=142;
inline constexpr size_t reserved_143=143;
inline constexpr size_t reserved_144=144;
inline constexpr size_t reserved_145=145;
inline constexpr size_t reserved_146=146;
inline constexpr size_t reserved_147=147;
inline constexpr size_t reserved_148=148;
inline constexpr size_t reserved_149=149;
inline constexpr size_t reserved_150=150;
inline constexpr size_t reserved_151=151;
inline constexpr size_t reserved_152=152;
inline constexpr size_t reserved_153=153;
inline constexpr size_t reserved_154=154;
inline constexpr size_t reserved_155=155;
inline constexpr size_t reserved_156=156;
inline constexpr size_t reserved_157=157;
inline constexpr size_t reserved_158=158;
inline constexpr size_t reserved_159=159;
inline constexpr size_t reserved_160=160;
inline constexpr size_t reserved_161=161;
inline constexpr size_t reserved_162=162;
inline constexpr size_t reserved_163=163;
inline constexpr size_t reserved_164=164;
inline constexpr size_t reserved_165=165;
inline constexpr size_t reserved_166=166;
inline constexpr size_t reserved_167=167;
inline constexpr size_t reserved_168=168;
inline constexpr size_t reserved_169=169;
inline constexpr size_t reserved_170=170;
inline constexpr size_t reserved_171=171;
inline constexpr size_t reserved_172=172;
inline constexpr size_t reserved_173=173;
inline constexpr size_t reserved_174=174;
inline constexpr size_t reserved_175=175;
inline constexpr size_t reserved_176=176;
inline constexpr size_t reserved_177=177;
inline constexpr size_t reserved_178=178;
inline constexpr size_t reserved_179=179;
inline constexpr size_t reserved_180=180;
inline constexpr size_t reserved_181=181;
inline constexpr size_t reserved_182=182;
inline constexpr size_t reserved_183=183;
inline constexpr size_t reserved_184=184;
inline constexpr size_t reserved_185=185;
inline constexpr size_t reserved_186=186;
inline constexpr size_t reserved_187=187;
inline constexpr size_t reserved_188=188;
inline constexpr size_t reserved_189=189;
inline constexpr size_t reserved_190=190;
inline constexpr size_t reserved_191=191;
inline constexpr size_t reserved_192=192;
inline constexpr size_t reserved_193=193;
inline constexpr size_t reserved_194=194;
inline constexpr size_t reserved_195=195;
inline constexpr size_t reserved_196=196;
inline constexpr size_t reserved_197=197;
inline constexpr size_t reserved_198=198;
inline constexpr size_t reserved_199=199;
inline constexpr size_t reserved_200=200;
inline constexpr size_t reserved_201=201;
inline constexpr size_t reserved_202=202;
inline constexpr size_t reserved_203=203;
inline constexpr size_t reserved_204=204;
inline constexpr size_t reserved_205=205;
inline constexpr size_t reserved_206=206;
inline constexpr size_t reserved_207=207;
inline constexpr size_t reserved_208=208;
inline constexpr size_t reserved_209=209;
inline constexpr size_t reserved_210=210;
inline constexpr size_t reserved_211=211;
inline constexpr size_t reserved_212=212;
inline constexpr size_t reserved_213=213;
inline constexpr size_t reserved_214=214;
inline constexpr size_t reserved_215=215;
inline constexpr size_t reserved_216=216;
inline constexpr size_t reserved_217=217;
inline constexpr size_t reserved_218=218;
inline constexpr size_t reserved_219=219;
inline constexpr size_t reserved_220=220;
inline constexpr size_t reserved_221=221;
inline constexpr size_t reserved_222=222;
inline constexpr size_t reserved_223=223;
inline constexpr size_t reserved_224=224;
inline constexpr size_t reserved_225=225;
inline constexpr size_t reserved_226=226;
inline constexpr size_t reserved_227=227;
inline constexpr size_t reserved_228=228;
inline constexpr size_t reserved_229=229;
inline constexpr size_t reserved_230=230;
inline constexpr size_t reserved_231=231;
inline constexpr size_t reserved_232=232;
inline constexpr size_t reserved_233=233;
inline constexpr size_t reserved_234=234;
inline constexpr size_t reserved_235=235;
inline constexpr size_t reserved_236=236;
inline constexpr size_t reserved_237=237;
inline constexpr size_t reserved_238=238;
inline constexpr size_t reserved_239=239;
inline constexpr size_t reserved_240=240;
inline constexpr size_t reserved_241=241;
inline constexpr size_t reserved_242=242;
inline constexpr size_t reserved_243=243;
inline constexpr size_t reserved_244=244;
inline constexpr size_t reserved_245=245;
inline constexpr size_t reserved_246=246;
inline constexpr size_t reserved_247=247;
inline constexpr size_t reserved_248=248;
inline constexpr size_t reserved_249=249;
inline constexpr size_t reserved_250=250;
inline constexpr size_t reserved_251=251;
inline constexpr size_t reserved_252=252;
inline constexpr size_t reserved_253=253;
inline constexpr size_t reserved_254=254;
inline constexpr size_t reserved_255=255;
}
namespace column {
inline constexpr size_t S=0;
inline constexpr size_t F=1;
inline constexpr size_t pLo=2;
inline constexpr size_t pHi=3;
inline constexpr size_t mode=4;
inline constexpr size_t restTicks=5;
inline constexpr size_t bedDepth=6;
inline constexpr size_t flowOriginCount=7;
inline constexpr size_t Psum=8;
inline constexpr size_t B=9;
inline constexpr size_t activationRequested=10;
inline constexpr size_t activationAdmitted=11;
inline constexpr size_t depositReason=12;
inline constexpr size_t inputFlags=13;
inline constexpr size_t lastMutationLo=14;
inline constexpr size_t lastMutationHi=15;
inline constexpr size_t reserved_16=16;
inline constexpr size_t reserved_17=17;
inline constexpr size_t reserved_18=18;
inline constexpr size_t reserved_19=19;
inline constexpr size_t reserved_20=20;
inline constexpr size_t reserved_21=21;
inline constexpr size_t reserved_22=22;
inline constexpr size_t reserved_23=23;
inline constexpr size_t reserved_24=24;
inline constexpr size_t reserved_25=25;
inline constexpr size_t reserved_26=26;
inline constexpr size_t reserved_27=27;
inline constexpr size_t reserved_28=28;
inline constexpr size_t reserved_29=29;
inline constexpr size_t reserved_30=30;
inline constexpr size_t reserved_31=31;
}
namespace packet {
inline constexpr size_t mass=0;
inline constexpr size_t x=1;
inline constexpr size_t y2=2;
inline constexpr size_t vx=3;
inline constexpr size_t vy=4;
inline constexpr size_t column=5;
inline constexpr size_t id=6;
inline constexpr size_t generation=7;
inline constexpr size_t originLo=8;
inline constexpr size_t originHi=9;
inline constexpr size_t sourceId=10;
inline constexpr size_t birthMicro=11;
inline constexpr size_t flags=12;
inline constexpr size_t pathStart=13;
inline constexpr size_t pathCount=14;
inline constexpr size_t mutationLo=15;
inline constexpr size_t mutationHi=16;
inline constexpr size_t certificate_birthId=17;
inline constexpr size_t certificate_slotGeneration=18;
inline constexpr size_t certificate_originLo=19;
inline constexpr size_t certificate_originHi=20;
inline constexpr size_t certificate_sourceId=21;
inline constexpr size_t certificate_birthKind=22;
inline constexpr size_t certificate_birthCommitLo=23;
inline constexpr size_t certificate_birthCommitHi=24;
inline constexpr size_t certificate_birthOrdinal=25;
inline constexpr size_t certificate_birthMass=26;
inline constexpr size_t certificate_birthColumn=27;
inline constexpr size_t certificate_parentOriginRank=28;
inline constexpr size_t certificate_parentMassBefore=29;
inline constexpr size_t certificate_certificateRevision=30;
inline constexpr size_t certificate_reservedCert14=31;
inline constexpr size_t certificate_reservedCert15=32;
inline constexpr size_t reserved_33=33;
inline constexpr size_t reserved_34=34;
inline constexpr size_t reserved_35=35;
inline constexpr size_t reserved_36=36;
inline constexpr size_t reserved_37=37;
inline constexpr size_t reserved_38=38;
inline constexpr size_t reserved_39=39;
inline constexpr size_t reserved_40=40;
inline constexpr size_t reserved_41=41;
inline constexpr size_t reserved_42=42;
inline constexpr size_t reserved_43=43;
inline constexpr size_t reserved_44=44;
inline constexpr size_t reserved_45=45;
inline constexpr size_t reserved_46=46;
inline constexpr size_t reserved_47=47;
inline constexpr size_t reserved_48=48;
inline constexpr size_t reserved_49=49;
inline constexpr size_t reserved_50=50;
inline constexpr size_t reserved_51=51;
inline constexpr size_t reserved_52=52;
inline constexpr size_t reserved_53=53;
inline constexpr size_t reserved_54=54;
inline constexpr size_t reserved_55=55;
inline constexpr size_t reserved_56=56;
inline constexpr size_t reserved_57=57;
inline constexpr size_t reserved_58=58;
inline constexpr size_t reserved_59=59;
inline constexpr size_t reserved_60=60;
inline constexpr size_t reserved_61=61;
inline constexpr size_t reserved_62=62;
inline constexpr size_t reserved_63=63;
}
namespace source {
inline constexpr size_t originLo=0;
inline constexpr size_t originHi=1;
inline constexpr size_t sourceId=2;
inline constexpr size_t sourceKind=3;
inline constexpr size_t admittedLo=4;
inline constexpr size_t admittedHi=5;
inline constexpr size_t sourceSeqLo=6;
inline constexpr size_t sourceSeqHi=7;
inline constexpr size_t sourceInputLo=8;
inline constexpr size_t sourceInputHi=9;
inline constexpr size_t rawCommandDigest0=10;
inline constexpr size_t rawCommandDigest1=11;
inline constexpr size_t rawCommandDigest2=12;
inline constexpr size_t rawCommandDigest3=13;
inline constexpr size_t sourceRevision=14;
inline constexpr size_t flags=15;
}
namespace input {
inline constexpr size_t magic=0;
inline constexpr size_t revision=1;
inline constexpr size_t profile=2;
inline constexpr size_t flags=3;
inline constexpr size_t runLo=4;
inline constexpr size_t runHi=5;
inline constexpr size_t ownerLo=6;
inline constexpr size_t ownerHi=7;
inline constexpr size_t contextLo=8;
inline constexpr size_t contextHi=9;
inline constexpr size_t rootNonceLo=10;
inline constexpr size_t rootNonceHi=11;
inline constexpr size_t inputGenerationLo=12;
inline constexpr size_t inputGenerationHi=13;
inline constexpr size_t sampleSeqLo=14;
inline constexpr size_t sampleSeqHi=15;
inline constexpr size_t captureNsLo=16;
inline constexpr size_t captureNsHi=17;
inline constexpr size_t receiptNsLo=18;
inline constexpr size_t receiptNsHi=19;
inline constexpr size_t freezeNsLo=20;
inline constexpr size_t freezeNsHi=21;
inline constexpr size_t clockDomain=22;
inline constexpr size_t gravitySource=23;
inline constexpr size_t acceptedAngleIndex=24;
inline constexpr size_t parameterRevision=25;
inline constexpr size_t parameterSha0=26;
inline constexpr size_t parameterSha1=27;
inline constexpr size_t parameterSha2=28;
inline constexpr size_t parameterSha3=29;
inline constexpr size_t parameterSha4=30;
inline constexpr size_t parameterSha5=31;
inline constexpr size_t parameterSha6=32;
inline constexpr size_t parameterSha7=33;
inline constexpr size_t rawAngleLo=34;
inline constexpr size_t rawAngleHi=35;
inline constexpr size_t sensorAdapterRevision=36;
inline constexpr size_t sensorFlags=37;
inline constexpr size_t reservedEnvelope38=38;
inline constexpr size_t reservedEnvelope39=39;
inline constexpr size_t commandKind=40;
inline constexpr size_t commandSeqLo=41;
inline constexpr size_t commandSeqHi=42;
inline constexpr size_t commandCaptureNsLo=43;
inline constexpr size_t commandCaptureNsHi=44;
inline constexpr size_t commandFlags=45;
inline constexpr size_t rawTouchXLo=46;
inline constexpr size_t rawTouchXHi=47;
inline constexpr size_t rawRadiusLo=48;
inline constexpr size_t rawRadiusHi=49;
inline constexpr size_t rawGestureDxLo=50;
inline constexpr size_t rawGestureDxHi=51;
inline constexpr size_t rawGestureDyLo=52;
inline constexpr size_t rawGestureDyHi=53;
inline constexpr size_t gestureDurationNsLo=54;
inline constexpr size_t gestureDurationNsHi=55;
inline constexpr size_t physicalJxLo=56;
inline constexpr size_t physicalJxHi=57;
inline constexpr size_t physicalJyLo=58;
inline constexpr size_t physicalJyHi=59;
inline constexpr size_t workCap0=60;
inline constexpr size_t workCap1=61;
inline constexpr size_t workCap2=62;
inline constexpr size_t workCap3=63;
inline constexpr size_t rawSourceMassLo=64;
inline constexpr size_t rawSourceMassHi=65;
inline constexpr size_t rawSourceXLo=66;
inline constexpr size_t rawSourceXHi=67;
inline constexpr size_t rawSourceYLo=68;
inline constexpr size_t rawSourceYHi=69;
inline constexpr size_t rawSourceVxLo=70;
inline constexpr size_t rawSourceVxHi=71;
inline constexpr size_t rawSourceVyLo=72;
inline constexpr size_t rawSourceVyHi=73;
inline constexpr size_t sourceId=74;
inline constexpr size_t sourceKind=75;
inline constexpr size_t sourceAdapterRevision=76;
inline constexpr size_t touchAdapterRevision=77;
inline constexpr size_t reserved_78=78;
inline constexpr size_t reserved_79=79;
inline constexpr size_t reserved_80=80;
inline constexpr size_t reserved_81=81;
inline constexpr size_t reserved_82=82;
inline constexpr size_t reserved_83=83;
inline constexpr size_t reserved_84=84;
inline constexpr size_t reserved_85=85;
inline constexpr size_t reserved_86=86;
inline constexpr size_t reserved_87=87;
inline constexpr size_t reserved_88=88;
inline constexpr size_t reserved_89=89;
inline constexpr size_t reserved_90=90;
inline constexpr size_t reserved_91=91;
inline constexpr size_t reserved_92=92;
inline constexpr size_t reserved_93=93;
inline constexpr size_t reserved_94=94;
inline constexpr size_t reserved_95=95;
inline constexpr size_t reserved_96=96;
inline constexpr size_t reserved_97=97;
inline constexpr size_t reserved_98=98;
inline constexpr size_t reserved_99=99;
inline constexpr size_t reserved_100=100;
inline constexpr size_t reserved_101=101;
inline constexpr size_t reserved_102=102;
inline constexpr size_t reserved_103=103;
inline constexpr size_t reserved_104=104;
inline constexpr size_t reserved_105=105;
inline constexpr size_t reserved_106=106;
inline constexpr size_t reserved_107=107;
inline constexpr size_t reserved_108=108;
inline constexpr size_t reserved_109=109;
inline constexpr size_t reserved_110=110;
inline constexpr size_t reserved_111=111;
inline constexpr size_t reserved_112=112;
inline constexpr size_t reserved_113=113;
inline constexpr size_t reserved_114=114;
inline constexpr size_t reserved_115=115;
inline constexpr size_t reserved_116=116;
inline constexpr size_t reserved_117=117;
inline constexpr size_t reserved_118=118;
inline constexpr size_t reserved_119=119;
inline constexpr size_t reserved_120=120;
inline constexpr size_t reserved_121=121;
inline constexpr size_t reserved_122=122;
inline constexpr size_t reserved_123=123;
inline constexpr size_t reserved_124=124;
inline constexpr size_t reserved_125=125;
inline constexpr size_t reserved_126=126;
inline constexpr size_t reserved_127=127;
}
namespace completion {
inline constexpr size_t magic=0;
inline constexpr size_t revision=1;
inline constexpr size_t words=2;
inline constexpr size_t seal=3;
inline constexpr size_t profile=4;
inline constexpr size_t rootKind=5;
inline constexpr size_t outcome=6;
inline constexpr size_t reason=7;
inline constexpr size_t nonceLo=8;
inline constexpr size_t nonceHi=9;
inline constexpr size_t runLo=10;
inline constexpr size_t runHi=11;
inline constexpr size_t ownerLo=12;
inline constexpr size_t ownerHi=13;
inline constexpr size_t contextLo=14;
inline constexpr size_t contextHi=15;
inline constexpr size_t surfaceLo=16;
inline constexpr size_t surfaceHi=17;
inline constexpr size_t geometryLo=18;
inline constexpr size_t geometryHi=19;
inline constexpr size_t configLo=20;
inline constexpr size_t configHi=21;
inline constexpr size_t inputGenerationLo=22;
inline constexpr size_t inputGenerationHi=23;
inline constexpr size_t pageSessionLo=24;
inline constexpr size_t pageSessionHi=25;
inline constexpr size_t planIdLo=26;
inline constexpr size_t planIdHi=27;
inline constexpr size_t baseCommitLo=28;
inline constexpr size_t baseCommitHi=29;
inline constexpr size_t proposedCommitLo=30;
inline constexpr size_t proposedCommitHi=31;
inline constexpr size_t baseBank=32;
inline constexpr size_t candidateBank=33;
inline constexpr size_t arena=34;
inline constexpr size_t arenaGenerationLo=35;
inline constexpr size_t arenaGenerationHi=36;
inline constexpr size_t selectedJ=37;
inline constexpr size_t eligibleJ=38;
inline constexpr size_t completedDispatches=39;
inline constexpr size_t requiredDispatches=40;
inline constexpr size_t barriers=41;
inline constexpr size_t auditMask=42;
inline constexpr size_t eventPlanned=43;
inline constexpr size_t eventSealed=44;
inline constexpr size_t pathPlanned=45;
inline constexpr size_t pathSealed=46;
inline constexpr size_t trialValidMask=47;
inline constexpr size_t firstErrorStage=48;
inline constexpr size_t firstErrorMicro=49;
inline constexpr size_t firstErrorEntity=50;
inline constexpr size_t firstErrorOperand=51;
inline constexpr size_t originalGL=52;
inline constexpr size_t fenceStatus=53;
inline constexpr size_t acceptedQuantaLo=54;
inline constexpr size_t acceptedQuantaHi=55;
inline constexpr size_t totalMassLo=56;
inline constexpr size_t totalMassHi=57;
inline constexpr size_t sourceSeqLo=58;
inline constexpr size_t sourceSeqHi=59;
inline constexpr size_t impulseSeqLo=60;
inline constexpr size_t impulseSeqHi=61;
inline constexpr size_t sourceAdmittedLo=62;
inline constexpr size_t sourceAdmittedHi=63;
inline constexpr size_t parameterSha0=64;
inline constexpr size_t parameterSha1=65;
inline constexpr size_t parameterSha2=66;
inline constexpr size_t parameterSha3=67;
inline constexpr size_t parameterSha4=68;
inline constexpr size_t parameterSha5=69;
inline constexpr size_t parameterSha6=70;
inline constexpr size_t parameterSha7=71;
inline constexpr size_t layoutSha0=72;
inline constexpr size_t layoutSha1=73;
inline constexpr size_t layoutSha2=74;
inline constexpr size_t layoutSha3=75;
inline constexpr size_t layoutSha4=76;
inline constexpr size_t layoutSha5=77;
inline constexpr size_t layoutSha6=78;
inline constexpr size_t layoutSha7=79;
inline constexpr size_t shaderSha0=80;
inline constexpr size_t shaderSha1=81;
inline constexpr size_t shaderSha2=82;
inline constexpr size_t shaderSha3=83;
inline constexpr size_t shaderSha4=84;
inline constexpr size_t shaderSha5=85;
inline constexpr size_t shaderSha6=86;
inline constexpr size_t shaderSha7=87;
inline constexpr size_t inputSha0=88;
inline constexpr size_t inputSha1=89;
inline constexpr size_t inputSha2=90;
inline constexpr size_t inputSha3=91;
inline constexpr size_t inputSha4=92;
inline constexpr size_t inputSha5=93;
inline constexpr size_t inputSha6=94;
inline constexpr size_t inputSha7=95;
inline constexpr size_t movingPx0=96;
inline constexpr size_t movingPx1=97;
inline constexpr size_t movingPx2=98;
inline constexpr size_t movingPx3=99;
inline constexpr size_t movingPy0=100;
inline constexpr size_t movingPy1=101;
inline constexpr size_t movingPy2=102;
inline constexpr size_t movingPy3=103;
inline constexpr size_t gravityWork0=104;
inline constexpr size_t gravityWork1=105;
inline constexpr size_t gravityWork2=106;
inline constexpr size_t gravityWork3=107;
inline constexpr size_t touchWork0=108;
inline constexpr size_t touchWork1=109;
inline constexpr size_t touchWork2=110;
inline constexpr size_t touchWork3=111;
inline constexpr size_t sourceWork0=112;
inline constexpr size_t sourceWork1=113;
inline constexpr size_t sourceWork2=114;
inline constexpr size_t sourceWork3=115;
inline constexpr size_t dissipationLower0=116;
inline constexpr size_t dissipationLower1=117;
inline constexpr size_t dissipationLower2=118;
inline constexpr size_t dissipationLower3=119;
inline constexpr size_t trial0_J=120;
inline constexpr size_t trial0_outcome=121;
inline constexpr size_t trial0_firstStage=122;
inline constexpr size_t trial0_firstMicro=123;
inline constexpr size_t trial0_firstEntity=124;
inline constexpr size_t trial0_firstOperand=125;
inline constexpr size_t trial1_J=126;
inline constexpr size_t trial1_outcome=127;
inline constexpr size_t trial1_firstStage=128;
inline constexpr size_t trial1_firstMicro=129;
inline constexpr size_t trial1_firstEntity=130;
inline constexpr size_t trial1_firstOperand=131;
inline constexpr size_t trial2_J=132;
inline constexpr size_t trial2_outcome=133;
inline constexpr size_t trial2_firstStage=134;
inline constexpr size_t trial2_firstMicro=135;
inline constexpr size_t trial2_firstEntity=136;
inline constexpr size_t trial2_firstOperand=137;
inline constexpr size_t trial3_J=138;
inline constexpr size_t trial3_outcome=139;
inline constexpr size_t trial3_firstStage=140;
inline constexpr size_t trial3_firstMicro=141;
inline constexpr size_t trial3_firstEntity=142;
inline constexpr size_t trial3_firstOperand=143;
inline constexpr size_t trial4_J=144;
inline constexpr size_t trial4_outcome=145;
inline constexpr size_t trial4_firstStage=146;
inline constexpr size_t trial4_firstMicro=147;
inline constexpr size_t trial4_firstEntity=148;
inline constexpr size_t trial4_firstOperand=149;
inline constexpr size_t trial5_J=150;
inline constexpr size_t trial5_outcome=151;
inline constexpr size_t trial5_firstStage=152;
inline constexpr size_t trial5_firstMicro=153;
inline constexpr size_t trial5_firstEntity=154;
inline constexpr size_t trial5_firstOperand=155;
inline constexpr size_t reserved_156=156;
inline constexpr size_t reserved_157=157;
inline constexpr size_t reserved_158=158;
inline constexpr size_t reserved_159=159;
inline constexpr size_t reserved_160=160;
inline constexpr size_t reserved_161=161;
inline constexpr size_t reserved_162=162;
inline constexpr size_t reserved_163=163;
inline constexpr size_t reserved_164=164;
inline constexpr size_t reserved_165=165;
inline constexpr size_t reserved_166=166;
inline constexpr size_t reserved_167=167;
inline constexpr size_t reserved_168=168;
inline constexpr size_t reserved_169=169;
inline constexpr size_t reserved_170=170;
inline constexpr size_t reserved_171=171;
inline constexpr size_t reserved_172=172;
inline constexpr size_t reserved_173=173;
inline constexpr size_t reserved_174=174;
inline constexpr size_t reserved_175=175;
inline constexpr size_t reserved_176=176;
inline constexpr size_t reserved_177=177;
inline constexpr size_t reserved_178=178;
inline constexpr size_t reserved_179=179;
inline constexpr size_t reserved_180=180;
inline constexpr size_t reserved_181=181;
inline constexpr size_t reserved_182=182;
inline constexpr size_t reserved_183=183;
inline constexpr size_t reserved_184=184;
inline constexpr size_t reserved_185=185;
inline constexpr size_t reserved_186=186;
inline constexpr size_t reserved_187=187;
inline constexpr size_t reserved_188=188;
inline constexpr size_t reserved_189=189;
inline constexpr size_t reserved_190=190;
inline constexpr size_t reserved_191=191;
inline constexpr size_t reserved_192=192;
inline constexpr size_t reserved_193=193;
inline constexpr size_t reserved_194=194;
inline constexpr size_t reserved_195=195;
inline constexpr size_t reserved_196=196;
inline constexpr size_t reserved_197=197;
inline constexpr size_t reserved_198=198;
inline constexpr size_t reserved_199=199;
inline constexpr size_t reserved_200=200;
inline constexpr size_t reserved_201=201;
inline constexpr size_t reserved_202=202;
inline constexpr size_t reserved_203=203;
inline constexpr size_t reserved_204=204;
inline constexpr size_t reserved_205=205;
inline constexpr size_t reserved_206=206;
inline constexpr size_t reserved_207=207;
inline constexpr size_t reserved_208=208;
inline constexpr size_t reserved_209=209;
inline constexpr size_t reserved_210=210;
inline constexpr size_t reserved_211=211;
inline constexpr size_t reserved_212=212;
inline constexpr size_t reserved_213=213;
inline constexpr size_t reserved_214=214;
inline constexpr size_t reserved_215=215;
inline constexpr size_t reserved_216=216;
inline constexpr size_t reserved_217=217;
inline constexpr size_t reserved_218=218;
inline constexpr size_t reserved_219=219;
inline constexpr size_t reserved_220=220;
inline constexpr size_t reserved_221=221;
inline constexpr size_t reserved_222=222;
inline constexpr size_t reserved_223=223;
inline constexpr size_t reserved_224=224;
inline constexpr size_t reserved_225=225;
inline constexpr size_t reserved_226=226;
inline constexpr size_t reserved_227=227;
inline constexpr size_t reserved_228=228;
inline constexpr size_t reserved_229=229;
inline constexpr size_t reserved_230=230;
inline constexpr size_t reserved_231=231;
inline constexpr size_t reserved_232=232;
inline constexpr size_t reserved_233=233;
inline constexpr size_t reserved_234=234;
inline constexpr size_t reserved_235=235;
inline constexpr size_t reserved_236=236;
inline constexpr size_t reserved_237=237;
inline constexpr size_t reserved_238=238;
inline constexpr size_t reserved_239=239;
inline constexpr size_t reserved_240=240;
inline constexpr size_t reserved_241=241;
inline constexpr size_t reserved_242=242;
inline constexpr size_t reserved_243=243;
inline constexpr size_t reserved_244=244;
inline constexpr size_t reserved_245=245;
inline constexpr size_t reserved_246=246;
inline constexpr size_t reserved_247=247;
inline constexpr size_t reserved_248=248;
inline constexpr size_t reserved_249=249;
inline constexpr size_t reserved_250=250;
inline constexpr size_t reserved_251=251;
inline constexpr size_t reserved_252=252;
inline constexpr size_t reserved_253=253;
inline constexpr size_t reserved_254=254;
inline constexpr size_t reserved_255=255;
}
namespace mode {
inline constexpr uint32_t REST=0u;
inline constexpr uint32_t FLOW=1u;
}
namespace command {
inline constexpr uint32_t NONE=0u;
inline constexpr uint32_t SOURCE=1u;
inline constexpr uint32_t IMPULSE=2u;
}
namespace role {
inline constexpr uint32_t TRIAL=0u;
inline constexpr uint32_t AUTHORITATIVE=1u;
inline constexpr uint32_t REPLAY=2u;
}
namespace path {
inline constexpr uint32_t TURN_X=1u;
inline constexpr uint32_t TURN_Y=2u;
inline constexpr uint32_t CROSS=3u;
inline constexpr uint32_t TOP_CONTACT=4u;
inline constexpr uint32_t SIDE_CONTACT=5u;
inline constexpr uint32_t CEILING=6u;
inline constexpr uint32_t WALL=7u;
inline constexpr uint32_t END=8u;
}
namespace outcome {
inline constexpr uint32_t ACCEPTED=1u;
inline constexpr uint32_t ALREADY_CONSUMED=2u;
inline constexpr uint32_t SOURCE_ZERO_ADMISSION=3u;
inline constexpr uint32_t G9_NUMERIC_OR_CAPACITY_BLOCKED=4u;
inline constexpr uint32_t FAULT=5u;
}
namespace reason {
inline constexpr uint32_t OK=0u;
inline constexpr uint32_t NOOP=1u;
inline constexpr uint32_t NO_HIT=2u;
inline constexpr uint32_t NO_MOVABLE_MASS=3u;
inline constexpr uint32_t NUMERIC_RESOLUTION=4u;
inline constexpr uint32_t CFL=5u;
inline constexpr uint32_t VELOCITY_DOMAIN=6u;
inline constexpr uint32_t NEGATIVE_OR_EMPTY_WITH_MOMENTUM=7u;
inline constexpr uint32_t CAPTURE_CAPACITY=8u;
inline constexpr uint32_t EMBEDDED_PACKET=9u;
inline constexpr uint32_t SUPPORT_MOVED_DEFER=10u;
inline constexpr uint32_t FLOW_ORIGIN_CAPACITY=11u;
inline constexpr uint32_t DEPOSIT_LAYER_WAIT=12u;
inline constexpr uint32_t POOL_FULL=13u;
inline constexpr uint32_t ID_EXHAUSTED=14u;
inline constexpr uint32_t SOURCE_ORIGIN_FULL=15u;
inline constexpr uint32_t ENERGY_INCREASE=16u;
inline constexpr uint32_t ENERGY_RESOLUTION=17u;
inline constexpr uint32_t WORK_CAP=18u;
inline constexpr uint32_t BAD_DUPLICATE=19u;
inline constexpr uint32_t STALE_COMMAND=20u;
inline constexpr uint32_t STALE_INPUT=21u;
inline constexpr uint32_t BAD_IDENTITY=22u;
inline constexpr uint32_t BAD_VERSION=23u;
inline constexpr uint32_t BAD_ORIGIN=24u;
inline constexpr uint32_t BAD_REPLAY=25u;
inline constexpr uint32_t BAD_SEAL=26u;
inline constexpr uint32_t EVENT_CREDIT=27u;
inline constexpr uint32_t PATH_BOUND=28u;
inline constexpr uint32_t OVERFLOW=29u;
inline constexpr uint32_t STOP_OR_CONTEXT_LOST=30u;
}
inline constexpr uint32_t kEventROOT_BEGIN=1u;
inline constexpr uint32_t kEventGRAVITY_WORK=2u;
inline constexpr uint32_t kEventSOURCE_BIRTH=3u;
inline constexpr uint32_t kEventACTIVATE=4u;
inline constexpr uint32_t kEventTOUCH_WORK=5u;
inline constexpr uint32_t kEventFLIGHT=6u;
inline constexpr uint32_t kEventCAPTURE=7u;
inline constexpr uint32_t kEventEDGE_FLUX=8u;
inline constexpr uint32_t kEventMOMENTUM_FORCE=9u;
inline constexpr uint32_t kEventFRICTION=10u;
inline constexpr uint32_t kEventDEPOSIT=11u;
inline constexpr uint32_t kEventLAUNCH=12u;
inline constexpr uint32_t kEventSTEP_SEAL=13u;
inline constexpr uint32_t kEventROOT_END=14u;
enum class Op:uint32_t{ROOT_INIT,BASE_FREEZE,CONFIG_INPUT_GATE,TRIAL_SCHEDULE_BEGIN,INIT_COPY,INPUT_AND_CFL,GRAVITY_WORK,TOUCH_ATOMIC,AFTER_TOUCH_CFL,ACTIVATE_REQUEST,ACTIVATE_APPLY,FLIGHT_PATH,PARTICLE_FOLD,FACE_FLUX_MOBILITY,FLOW_ORIGIN_RESERVE,FLOW_PAIR_APPLY,FRICTION_REST_DEPOSIT,ORDERED_CAPTURE,MICRO_AUDIT_SEAL,FINAL_MASS_ORIGIN_MOMENTUM,FINAL_ENERGY_CURSORS,SCOPE_SEAL_AND_CHOICE,DISCOVERY_COMMIT_SELECTION,AUTHORITATIVE_BEGIN,REJECT_PREFIX_BEGIN,REPLAY_BEGIN,COMPARE_SELECTED_AUTHORITATIVE,COMPARE_REPLAY_STREAM,PACK_SCENE_METADATA,ROOT_SEAL,SOURCE_INIT_BASE,SOURCE_VALIDATE_INPUT,SOURCE_DEDUP,SOURCE_DECODE,SOURCE_CREDIT,SOURCE_PRIVATE_BIRTH,SOURCE_ORIGIN_CERTIFICATE,SOURCE_LEDGER,SOURCE_AUDIT_REPLAY,SOURCE_COMPARE,SOURCE_PACK_SCENE,SOURCE_SEAL};
struct Pass {Op op;uint32_t role,j;int32_t micro;};
inline constexpr std::array<Pass,2024> kQuantumPasses={{
{Op::ROOT_INIT,0,0,-1},
{Op::BASE_FREEZE,0,0,-1},
{Op::CONFIG_INPUT_GATE,0,0,-1},
{Op::TRIAL_SCHEDULE_BEGIN,0,0,-1},
{Op::INIT_COPY,1,1,-1},
{Op::INPUT_AND_CFL,1,1,-1},
{Op::GRAVITY_WORK,1,1,-1},
{Op::TOUCH_ATOMIC,1,1,-1},
{Op::AFTER_TOUCH_CFL,1,1,-1},
{Op::ACTIVATE_REQUEST,1,1,0},
{Op::ACTIVATE_APPLY,1,1,0},
{Op::FLIGHT_PATH,1,1,0},
{Op::PARTICLE_FOLD,1,1,0},
{Op::FACE_FLUX_MOBILITY,1,1,0},
{Op::FLOW_ORIGIN_RESERVE,1,1,0},
{Op::FLOW_PAIR_APPLY,1,1,0},
{Op::FRICTION_REST_DEPOSIT,1,1,0},
{Op::ORDERED_CAPTURE,1,1,0},
{Op::MICRO_AUDIT_SEAL,1,1,0},
{Op::FINAL_MASS_ORIGIN_MOMENTUM,1,1,-1},
{Op::FINAL_ENERGY_CURSORS,1,1,-1},
{Op::SCOPE_SEAL_AND_CHOICE,1,1,-1},
{Op::INIT_COPY,1,2,-1},
{Op::INPUT_AND_CFL,1,2,-1},
{Op::GRAVITY_WORK,1,2,-1},
{Op::TOUCH_ATOMIC,1,2,-1},
{Op::AFTER_TOUCH_CFL,1,2,-1},
{Op::ACTIVATE_REQUEST,1,2,0},
{Op::ACTIVATE_APPLY,1,2,0},
{Op::FLIGHT_PATH,1,2,0},
{Op::PARTICLE_FOLD,1,2,0},
{Op::FACE_FLUX_MOBILITY,1,2,0},
{Op::FLOW_ORIGIN_RESERVE,1,2,0},
{Op::FLOW_PAIR_APPLY,1,2,0},
{Op::FRICTION_REST_DEPOSIT,1,2,0},
{Op::ORDERED_CAPTURE,1,2,0},
{Op::MICRO_AUDIT_SEAL,1,2,0},
{Op::ACTIVATE_REQUEST,1,2,1},
{Op::ACTIVATE_APPLY,1,2,1},
{Op::FLIGHT_PATH,1,2,1},
{Op::PARTICLE_FOLD,1,2,1},
{Op::FACE_FLUX_MOBILITY,1,2,1},
{Op::FLOW_ORIGIN_RESERVE,1,2,1},
{Op::FLOW_PAIR_APPLY,1,2,1},
{Op::FRICTION_REST_DEPOSIT,1,2,1},
{Op::ORDERED_CAPTURE,1,2,1},
{Op::MICRO_AUDIT_SEAL,1,2,1},
{Op::FINAL_MASS_ORIGIN_MOMENTUM,1,2,-1},
{Op::FINAL_ENERGY_CURSORS,1,2,-1},
{Op::SCOPE_SEAL_AND_CHOICE,1,2,-1},
{Op::INIT_COPY,1,4,-1},
{Op::INPUT_AND_CFL,1,4,-1},
{Op::GRAVITY_WORK,1,4,-1},
{Op::TOUCH_ATOMIC,1,4,-1},
{Op::AFTER_TOUCH_CFL,1,4,-1},
{Op::ACTIVATE_REQUEST,1,4,0},
{Op::ACTIVATE_APPLY,1,4,0},
{Op::FLIGHT_PATH,1,4,0},
{Op::PARTICLE_FOLD,1,4,0},
{Op::FACE_FLUX_MOBILITY,1,4,0},
{Op::FLOW_ORIGIN_RESERVE,1,4,0},
{Op::FLOW_PAIR_APPLY,1,4,0},
{Op::FRICTION_REST_DEPOSIT,1,4,0},
{Op::ORDERED_CAPTURE,1,4,0},
{Op::MICRO_AUDIT_SEAL,1,4,0},
{Op::ACTIVATE_REQUEST,1,4,1},
{Op::ACTIVATE_APPLY,1,4,1},
{Op::FLIGHT_PATH,1,4,1},
{Op::PARTICLE_FOLD,1,4,1},
{Op::FACE_FLUX_MOBILITY,1,4,1},
{Op::FLOW_ORIGIN_RESERVE,1,4,1},
{Op::FLOW_PAIR_APPLY,1,4,1},
{Op::FRICTION_REST_DEPOSIT,1,4,1},
{Op::ORDERED_CAPTURE,1,4,1},
{Op::MICRO_AUDIT_SEAL,1,4,1},
{Op::ACTIVATE_REQUEST,1,4,2},
{Op::ACTIVATE_APPLY,1,4,2},
{Op::FLIGHT_PATH,1,4,2},
{Op::PARTICLE_FOLD,1,4,2},
{Op::FACE_FLUX_MOBILITY,1,4,2},
{Op::FLOW_ORIGIN_RESERVE,1,4,2},
{Op::FLOW_PAIR_APPLY,1,4,2},
{Op::FRICTION_REST_DEPOSIT,1,4,2},
{Op::ORDERED_CAPTURE,1,4,2},
{Op::MICRO_AUDIT_SEAL,1,4,2},
{Op::ACTIVATE_REQUEST,1,4,3},
{Op::ACTIVATE_APPLY,1,4,3},
{Op::FLIGHT_PATH,1,4,3},
{Op::PARTICLE_FOLD,1,4,3},
{Op::FACE_FLUX_MOBILITY,1,4,3},
{Op::FLOW_ORIGIN_RESERVE,1,4,3},
{Op::FLOW_PAIR_APPLY,1,4,3},
{Op::FRICTION_REST_DEPOSIT,1,4,3},
{Op::ORDERED_CAPTURE,1,4,3},
{Op::MICRO_AUDIT_SEAL,1,4,3},
{Op::FINAL_MASS_ORIGIN_MOMENTUM,1,4,-1},
{Op::FINAL_ENERGY_CURSORS,1,4,-1},
{Op::SCOPE_SEAL_AND_CHOICE,1,4,-1},
{Op::INIT_COPY,1,8,-1},
{Op::INPUT_AND_CFL,1,8,-1},
{Op::GRAVITY_WORK,1,8,-1},
{Op::TOUCH_ATOMIC,1,8,-1},
{Op::AFTER_TOUCH_CFL,1,8,-1},
{Op::ACTIVATE_REQUEST,1,8,0},
{Op::ACTIVATE_APPLY,1,8,0},
{Op::FLIGHT_PATH,1,8,0},
{Op::PARTICLE_FOLD,1,8,0},
{Op::FACE_FLUX_MOBILITY,1,8,0},
{Op::FLOW_ORIGIN_RESERVE,1,8,0},
{Op::FLOW_PAIR_APPLY,1,8,0},
{Op::FRICTION_REST_DEPOSIT,1,8,0},
{Op::ORDERED_CAPTURE,1,8,0},
{Op::MICRO_AUDIT_SEAL,1,8,0},
{Op::ACTIVATE_REQUEST,1,8,1},
{Op::ACTIVATE_APPLY,1,8,1},
{Op::FLIGHT_PATH,1,8,1},
{Op::PARTICLE_FOLD,1,8,1},
{Op::FACE_FLUX_MOBILITY,1,8,1},
{Op::FLOW_ORIGIN_RESERVE,1,8,1},
{Op::FLOW_PAIR_APPLY,1,8,1},
{Op::FRICTION_REST_DEPOSIT,1,8,1},
{Op::ORDERED_CAPTURE,1,8,1},
{Op::MICRO_AUDIT_SEAL,1,8,1},
{Op::ACTIVATE_REQUEST,1,8,2},
{Op::ACTIVATE_APPLY,1,8,2},
{Op::FLIGHT_PATH,1,8,2},
{Op::PARTICLE_FOLD,1,8,2},
{Op::FACE_FLUX_MOBILITY,1,8,2},
{Op::FLOW_ORIGIN_RESERVE,1,8,2},
{Op::FLOW_PAIR_APPLY,1,8,2},
{Op::FRICTION_REST_DEPOSIT,1,8,2},
{Op::ORDERED_CAPTURE,1,8,2},
{Op::MICRO_AUDIT_SEAL,1,8,2},
{Op::ACTIVATE_REQUEST,1,8,3},
{Op::ACTIVATE_APPLY,1,8,3},
{Op::FLIGHT_PATH,1,8,3},
{Op::PARTICLE_FOLD,1,8,3},
{Op::FACE_FLUX_MOBILITY,1,8,3},
{Op::FLOW_ORIGIN_RESERVE,1,8,3},
{Op::FLOW_PAIR_APPLY,1,8,3},
{Op::FRICTION_REST_DEPOSIT,1,8,3},
{Op::ORDERED_CAPTURE,1,8,3},
{Op::MICRO_AUDIT_SEAL,1,8,3},
{Op::ACTIVATE_REQUEST,1,8,4},
{Op::ACTIVATE_APPLY,1,8,4},
{Op::FLIGHT_PATH,1,8,4},
{Op::PARTICLE_FOLD,1,8,4},
{Op::FACE_FLUX_MOBILITY,1,8,4},
{Op::FLOW_ORIGIN_RESERVE,1,8,4},
{Op::FLOW_PAIR_APPLY,1,8,4},
{Op::FRICTION_REST_DEPOSIT,1,8,4},
{Op::ORDERED_CAPTURE,1,8,4},
{Op::MICRO_AUDIT_SEAL,1,8,4},
{Op::ACTIVATE_REQUEST,1,8,5},
{Op::ACTIVATE_APPLY,1,8,5},
{Op::FLIGHT_PATH,1,8,5},
{Op::PARTICLE_FOLD,1,8,5},
{Op::FACE_FLUX_MOBILITY,1,8,5},
{Op::FLOW_ORIGIN_RESERVE,1,8,5},
{Op::FLOW_PAIR_APPLY,1,8,5},
{Op::FRICTION_REST_DEPOSIT,1,8,5},
{Op::ORDERED_CAPTURE,1,8,5},
{Op::MICRO_AUDIT_SEAL,1,8,5},
{Op::ACTIVATE_REQUEST,1,8,6},
{Op::ACTIVATE_APPLY,1,8,6},
{Op::FLIGHT_PATH,1,8,6},
{Op::PARTICLE_FOLD,1,8,6},
{Op::FACE_FLUX_MOBILITY,1,8,6},
{Op::FLOW_ORIGIN_RESERVE,1,8,6},
{Op::FLOW_PAIR_APPLY,1,8,6},
{Op::FRICTION_REST_DEPOSIT,1,8,6},
{Op::ORDERED_CAPTURE,1,8,6},
{Op::MICRO_AUDIT_SEAL,1,8,6},
{Op::ACTIVATE_REQUEST,1,8,7},
{Op::ACTIVATE_APPLY,1,8,7},
{Op::FLIGHT_PATH,1,8,7},
{Op::PARTICLE_FOLD,1,8,7},
{Op::FACE_FLUX_MOBILITY,1,8,7},
{Op::FLOW_ORIGIN_RESERVE,1,8,7},
{Op::FLOW_PAIR_APPLY,1,8,7},
{Op::FRICTION_REST_DEPOSIT,1,8,7},
{Op::ORDERED_CAPTURE,1,8,7},
{Op::MICRO_AUDIT_SEAL,1,8,7},
{Op::FINAL_MASS_ORIGIN_MOMENTUM,1,8,-1},
{Op::FINAL_ENERGY_CURSORS,1,8,-1},
{Op::SCOPE_SEAL_AND_CHOICE,1,8,-1},
{Op::INIT_COPY,1,16,-1},
{Op::INPUT_AND_CFL,1,16,-1},
{Op::GRAVITY_WORK,1,16,-1},
{Op::TOUCH_ATOMIC,1,16,-1},
{Op::AFTER_TOUCH_CFL,1,16,-1},
{Op::ACTIVATE_REQUEST,1,16,0},
{Op::ACTIVATE_APPLY,1,16,0},
{Op::FLIGHT_PATH,1,16,0},
{Op::PARTICLE_FOLD,1,16,0},
{Op::FACE_FLUX_MOBILITY,1,16,0},
{Op::FLOW_ORIGIN_RESERVE,1,16,0},
{Op::FLOW_PAIR_APPLY,1,16,0},
{Op::FRICTION_REST_DEPOSIT,1,16,0},
{Op::ORDERED_CAPTURE,1,16,0},
{Op::MICRO_AUDIT_SEAL,1,16,0},
{Op::ACTIVATE_REQUEST,1,16,1},
{Op::ACTIVATE_APPLY,1,16,1},
{Op::FLIGHT_PATH,1,16,1},
{Op::PARTICLE_FOLD,1,16,1},
{Op::FACE_FLUX_MOBILITY,1,16,1},
{Op::FLOW_ORIGIN_RESERVE,1,16,1},
{Op::FLOW_PAIR_APPLY,1,16,1},
{Op::FRICTION_REST_DEPOSIT,1,16,1},
{Op::ORDERED_CAPTURE,1,16,1},
{Op::MICRO_AUDIT_SEAL,1,16,1},
{Op::ACTIVATE_REQUEST,1,16,2},
{Op::ACTIVATE_APPLY,1,16,2},
{Op::FLIGHT_PATH,1,16,2},
{Op::PARTICLE_FOLD,1,16,2},
{Op::FACE_FLUX_MOBILITY,1,16,2},
{Op::FLOW_ORIGIN_RESERVE,1,16,2},
{Op::FLOW_PAIR_APPLY,1,16,2},
{Op::FRICTION_REST_DEPOSIT,1,16,2},
{Op::ORDERED_CAPTURE,1,16,2},
{Op::MICRO_AUDIT_SEAL,1,16,2},
{Op::ACTIVATE_REQUEST,1,16,3},
{Op::ACTIVATE_APPLY,1,16,3},
{Op::FLIGHT_PATH,1,16,3},
{Op::PARTICLE_FOLD,1,16,3},
{Op::FACE_FLUX_MOBILITY,1,16,3},
{Op::FLOW_ORIGIN_RESERVE,1,16,3},
{Op::FLOW_PAIR_APPLY,1,16,3},
{Op::FRICTION_REST_DEPOSIT,1,16,3},
{Op::ORDERED_CAPTURE,1,16,3},
{Op::MICRO_AUDIT_SEAL,1,16,3},
{Op::ACTIVATE_REQUEST,1,16,4},
{Op::ACTIVATE_APPLY,1,16,4},
{Op::FLIGHT_PATH,1,16,4},
{Op::PARTICLE_FOLD,1,16,4},
{Op::FACE_FLUX_MOBILITY,1,16,4},
{Op::FLOW_ORIGIN_RESERVE,1,16,4},
{Op::FLOW_PAIR_APPLY,1,16,4},
{Op::FRICTION_REST_DEPOSIT,1,16,4},
{Op::ORDERED_CAPTURE,1,16,4},
{Op::MICRO_AUDIT_SEAL,1,16,4},
{Op::ACTIVATE_REQUEST,1,16,5},
{Op::ACTIVATE_APPLY,1,16,5},
{Op::FLIGHT_PATH,1,16,5},
{Op::PARTICLE_FOLD,1,16,5},
{Op::FACE_FLUX_MOBILITY,1,16,5},
{Op::FLOW_ORIGIN_RESERVE,1,16,5},
{Op::FLOW_PAIR_APPLY,1,16,5},
{Op::FRICTION_REST_DEPOSIT,1,16,5},
{Op::ORDERED_CAPTURE,1,16,5},
{Op::MICRO_AUDIT_SEAL,1,16,5},
{Op::ACTIVATE_REQUEST,1,16,6},
{Op::ACTIVATE_APPLY,1,16,6},
{Op::FLIGHT_PATH,1,16,6},
{Op::PARTICLE_FOLD,1,16,6},
{Op::FACE_FLUX_MOBILITY,1,16,6},
{Op::FLOW_ORIGIN_RESERVE,1,16,6},
{Op::FLOW_PAIR_APPLY,1,16,6},
{Op::FRICTION_REST_DEPOSIT,1,16,6},
{Op::ORDERED_CAPTURE,1,16,6},
{Op::MICRO_AUDIT_SEAL,1,16,6},
{Op::ACTIVATE_REQUEST,1,16,7},
{Op::ACTIVATE_APPLY,1,16,7},
{Op::FLIGHT_PATH,1,16,7},
{Op::PARTICLE_FOLD,1,16,7},
{Op::FACE_FLUX_MOBILITY,1,16,7},
{Op::FLOW_ORIGIN_RESERVE,1,16,7},
{Op::FLOW_PAIR_APPLY,1,16,7},
{Op::FRICTION_REST_DEPOSIT,1,16,7},
{Op::ORDERED_CAPTURE,1,16,7},
{Op::MICRO_AUDIT_SEAL,1,16,7},
{Op::ACTIVATE_REQUEST,1,16,8},
{Op::ACTIVATE_APPLY,1,16,8},
{Op::FLIGHT_PATH,1,16,8},
{Op::PARTICLE_FOLD,1,16,8},
{Op::FACE_FLUX_MOBILITY,1,16,8},
{Op::FLOW_ORIGIN_RESERVE,1,16,8},
{Op::FLOW_PAIR_APPLY,1,16,8},
{Op::FRICTION_REST_DEPOSIT,1,16,8},
{Op::ORDERED_CAPTURE,1,16,8},
{Op::MICRO_AUDIT_SEAL,1,16,8},
{Op::ACTIVATE_REQUEST,1,16,9},
{Op::ACTIVATE_APPLY,1,16,9},
{Op::FLIGHT_PATH,1,16,9},
{Op::PARTICLE_FOLD,1,16,9},
{Op::FACE_FLUX_MOBILITY,1,16,9},
{Op::FLOW_ORIGIN_RESERVE,1,16,9},
{Op::FLOW_PAIR_APPLY,1,16,9},
{Op::FRICTION_REST_DEPOSIT,1,16,9},
{Op::ORDERED_CAPTURE,1,16,9},
{Op::MICRO_AUDIT_SEAL,1,16,9},
{Op::ACTIVATE_REQUEST,1,16,10},
{Op::ACTIVATE_APPLY,1,16,10},
{Op::FLIGHT_PATH,1,16,10},
{Op::PARTICLE_FOLD,1,16,10},
{Op::FACE_FLUX_MOBILITY,1,16,10},
{Op::FLOW_ORIGIN_RESERVE,1,16,10},
{Op::FLOW_PAIR_APPLY,1,16,10},
{Op::FRICTION_REST_DEPOSIT,1,16,10},
{Op::ORDERED_CAPTURE,1,16,10},
{Op::MICRO_AUDIT_SEAL,1,16,10},
{Op::ACTIVATE_REQUEST,1,16,11},
{Op::ACTIVATE_APPLY,1,16,11},
{Op::FLIGHT_PATH,1,16,11},
{Op::PARTICLE_FOLD,1,16,11},
{Op::FACE_FLUX_MOBILITY,1,16,11},
{Op::FLOW_ORIGIN_RESERVE,1,16,11},
{Op::FLOW_PAIR_APPLY,1,16,11},
{Op::FRICTION_REST_DEPOSIT,1,16,11},
{Op::ORDERED_CAPTURE,1,16,11},
{Op::MICRO_AUDIT_SEAL,1,16,11},
{Op::ACTIVATE_REQUEST,1,16,12},
{Op::ACTIVATE_APPLY,1,16,12},
{Op::FLIGHT_PATH,1,16,12},
{Op::PARTICLE_FOLD,1,16,12},
{Op::FACE_FLUX_MOBILITY,1,16,12},
{Op::FLOW_ORIGIN_RESERVE,1,16,12},
{Op::FLOW_PAIR_APPLY,1,16,12},
{Op::FRICTION_REST_DEPOSIT,1,16,12},
{Op::ORDERED_CAPTURE,1,16,12},
{Op::MICRO_AUDIT_SEAL,1,16,12},
{Op::ACTIVATE_REQUEST,1,16,13},
{Op::ACTIVATE_APPLY,1,16,13},
{Op::FLIGHT_PATH,1,16,13},
{Op::PARTICLE_FOLD,1,16,13},
{Op::FACE_FLUX_MOBILITY,1,16,13},
{Op::FLOW_ORIGIN_RESERVE,1,16,13},
{Op::FLOW_PAIR_APPLY,1,16,13},
{Op::FRICTION_REST_DEPOSIT,1,16,13},
{Op::ORDERED_CAPTURE,1,16,13},
{Op::MICRO_AUDIT_SEAL,1,16,13},
{Op::ACTIVATE_REQUEST,1,16,14},
{Op::ACTIVATE_APPLY,1,16,14},
{Op::FLIGHT_PATH,1,16,14},
{Op::PARTICLE_FOLD,1,16,14},
{Op::FACE_FLUX_MOBILITY,1,16,14},
{Op::FLOW_ORIGIN_RESERVE,1,16,14},
{Op::FLOW_PAIR_APPLY,1,16,14},
{Op::FRICTION_REST_DEPOSIT,1,16,14},
{Op::ORDERED_CAPTURE,1,16,14},
{Op::MICRO_AUDIT_SEAL,1,16,14},
{Op::ACTIVATE_REQUEST,1,16,15},
{Op::ACTIVATE_APPLY,1,16,15},
{Op::FLIGHT_PATH,1,16,15},
{Op::PARTICLE_FOLD,1,16,15},
{Op::FACE_FLUX_MOBILITY,1,16,15},
{Op::FLOW_ORIGIN_RESERVE,1,16,15},
{Op::FLOW_PAIR_APPLY,1,16,15},
{Op::FRICTION_REST_DEPOSIT,1,16,15},
{Op::ORDERED_CAPTURE,1,16,15},
{Op::MICRO_AUDIT_SEAL,1,16,15},
{Op::FINAL_MASS_ORIGIN_MOMENTUM,1,16,-1},
{Op::FINAL_ENERGY_CURSORS,1,16,-1},
{Op::SCOPE_SEAL_AND_CHOICE,1,16,-1},
{Op::INIT_COPY,1,32,-1},
{Op::INPUT_AND_CFL,1,32,-1},
{Op::GRAVITY_WORK,1,32,-1},
{Op::TOUCH_ATOMIC,1,32,-1},
{Op::AFTER_TOUCH_CFL,1,32,-1},
{Op::ACTIVATE_REQUEST,1,32,0},
{Op::ACTIVATE_APPLY,1,32,0},
{Op::FLIGHT_PATH,1,32,0},
{Op::PARTICLE_FOLD,1,32,0},
{Op::FACE_FLUX_MOBILITY,1,32,0},
{Op::FLOW_ORIGIN_RESERVE,1,32,0},
{Op::FLOW_PAIR_APPLY,1,32,0},
{Op::FRICTION_REST_DEPOSIT,1,32,0},
{Op::ORDERED_CAPTURE,1,32,0},
{Op::MICRO_AUDIT_SEAL,1,32,0},
{Op::ACTIVATE_REQUEST,1,32,1},
{Op::ACTIVATE_APPLY,1,32,1},
{Op::FLIGHT_PATH,1,32,1},
{Op::PARTICLE_FOLD,1,32,1},
{Op::FACE_FLUX_MOBILITY,1,32,1},
{Op::FLOW_ORIGIN_RESERVE,1,32,1},
{Op::FLOW_PAIR_APPLY,1,32,1},
{Op::FRICTION_REST_DEPOSIT,1,32,1},
{Op::ORDERED_CAPTURE,1,32,1},
{Op::MICRO_AUDIT_SEAL,1,32,1},
{Op::ACTIVATE_REQUEST,1,32,2},
{Op::ACTIVATE_APPLY,1,32,2},
{Op::FLIGHT_PATH,1,32,2},
{Op::PARTICLE_FOLD,1,32,2},
{Op::FACE_FLUX_MOBILITY,1,32,2},
{Op::FLOW_ORIGIN_RESERVE,1,32,2},
{Op::FLOW_PAIR_APPLY,1,32,2},
{Op::FRICTION_REST_DEPOSIT,1,32,2},
{Op::ORDERED_CAPTURE,1,32,2},
{Op::MICRO_AUDIT_SEAL,1,32,2},
{Op::ACTIVATE_REQUEST,1,32,3},
{Op::ACTIVATE_APPLY,1,32,3},
{Op::FLIGHT_PATH,1,32,3},
{Op::PARTICLE_FOLD,1,32,3},
{Op::FACE_FLUX_MOBILITY,1,32,3},
{Op::FLOW_ORIGIN_RESERVE,1,32,3},
{Op::FLOW_PAIR_APPLY,1,32,3},
{Op::FRICTION_REST_DEPOSIT,1,32,3},
{Op::ORDERED_CAPTURE,1,32,3},
{Op::MICRO_AUDIT_SEAL,1,32,3},
{Op::ACTIVATE_REQUEST,1,32,4},
{Op::ACTIVATE_APPLY,1,32,4},
{Op::FLIGHT_PATH,1,32,4},
{Op::PARTICLE_FOLD,1,32,4},
{Op::FACE_FLUX_MOBILITY,1,32,4},
{Op::FLOW_ORIGIN_RESERVE,1,32,4},
{Op::FLOW_PAIR_APPLY,1,32,4},
{Op::FRICTION_REST_DEPOSIT,1,32,4},
{Op::ORDERED_CAPTURE,1,32,4},
{Op::MICRO_AUDIT_SEAL,1,32,4},
{Op::ACTIVATE_REQUEST,1,32,5},
{Op::ACTIVATE_APPLY,1,32,5},
{Op::FLIGHT_PATH,1,32,5},
{Op::PARTICLE_FOLD,1,32,5},
{Op::FACE_FLUX_MOBILITY,1,32,5},
{Op::FLOW_ORIGIN_RESERVE,1,32,5},
{Op::FLOW_PAIR_APPLY,1,32,5},
{Op::FRICTION_REST_DEPOSIT,1,32,5},
{Op::ORDERED_CAPTURE,1,32,5},
{Op::MICRO_AUDIT_SEAL,1,32,5},
{Op::ACTIVATE_REQUEST,1,32,6},
{Op::ACTIVATE_APPLY,1,32,6},
{Op::FLIGHT_PATH,1,32,6},
{Op::PARTICLE_FOLD,1,32,6},
{Op::FACE_FLUX_MOBILITY,1,32,6},
{Op::FLOW_ORIGIN_RESERVE,1,32,6},
{Op::FLOW_PAIR_APPLY,1,32,6},
{Op::FRICTION_REST_DEPOSIT,1,32,6},
{Op::ORDERED_CAPTURE,1,32,6},
{Op::MICRO_AUDIT_SEAL,1,32,6},
{Op::ACTIVATE_REQUEST,1,32,7},
{Op::ACTIVATE_APPLY,1,32,7},
{Op::FLIGHT_PATH,1,32,7},
{Op::PARTICLE_FOLD,1,32,7},
{Op::FACE_FLUX_MOBILITY,1,32,7},
{Op::FLOW_ORIGIN_RESERVE,1,32,7},
{Op::FLOW_PAIR_APPLY,1,32,7},
{Op::FRICTION_REST_DEPOSIT,1,32,7},
{Op::ORDERED_CAPTURE,1,32,7},
{Op::MICRO_AUDIT_SEAL,1,32,7},
{Op::ACTIVATE_REQUEST,1,32,8},
{Op::ACTIVATE_APPLY,1,32,8},
{Op::FLIGHT_PATH,1,32,8},
{Op::PARTICLE_FOLD,1,32,8},
{Op::FACE_FLUX_MOBILITY,1,32,8},
{Op::FLOW_ORIGIN_RESERVE,1,32,8},
{Op::FLOW_PAIR_APPLY,1,32,8},
{Op::FRICTION_REST_DEPOSIT,1,32,8},
{Op::ORDERED_CAPTURE,1,32,8},
{Op::MICRO_AUDIT_SEAL,1,32,8},
{Op::ACTIVATE_REQUEST,1,32,9},
{Op::ACTIVATE_APPLY,1,32,9},
{Op::FLIGHT_PATH,1,32,9},
{Op::PARTICLE_FOLD,1,32,9},
{Op::FACE_FLUX_MOBILITY,1,32,9},
{Op::FLOW_ORIGIN_RESERVE,1,32,9},
{Op::FLOW_PAIR_APPLY,1,32,9},
{Op::FRICTION_REST_DEPOSIT,1,32,9},
{Op::ORDERED_CAPTURE,1,32,9},
{Op::MICRO_AUDIT_SEAL,1,32,9},
{Op::ACTIVATE_REQUEST,1,32,10},
{Op::ACTIVATE_APPLY,1,32,10},
{Op::FLIGHT_PATH,1,32,10},
{Op::PARTICLE_FOLD,1,32,10},
{Op::FACE_FLUX_MOBILITY,1,32,10},
{Op::FLOW_ORIGIN_RESERVE,1,32,10},
{Op::FLOW_PAIR_APPLY,1,32,10},
{Op::FRICTION_REST_DEPOSIT,1,32,10},
{Op::ORDERED_CAPTURE,1,32,10},
{Op::MICRO_AUDIT_SEAL,1,32,10},
{Op::ACTIVATE_REQUEST,1,32,11},
{Op::ACTIVATE_APPLY,1,32,11},
{Op::FLIGHT_PATH,1,32,11},
{Op::PARTICLE_FOLD,1,32,11},
{Op::FACE_FLUX_MOBILITY,1,32,11},
{Op::FLOW_ORIGIN_RESERVE,1,32,11},
{Op::FLOW_PAIR_APPLY,1,32,11},
{Op::FRICTION_REST_DEPOSIT,1,32,11},
{Op::ORDERED_CAPTURE,1,32,11},
{Op::MICRO_AUDIT_SEAL,1,32,11},
{Op::ACTIVATE_REQUEST,1,32,12},
{Op::ACTIVATE_APPLY,1,32,12},
{Op::FLIGHT_PATH,1,32,12},
{Op::PARTICLE_FOLD,1,32,12},
{Op::FACE_FLUX_MOBILITY,1,32,12},
{Op::FLOW_ORIGIN_RESERVE,1,32,12},
{Op::FLOW_PAIR_APPLY,1,32,12},
{Op::FRICTION_REST_DEPOSIT,1,32,12},
{Op::ORDERED_CAPTURE,1,32,12},
{Op::MICRO_AUDIT_SEAL,1,32,12},
{Op::ACTIVATE_REQUEST,1,32,13},
{Op::ACTIVATE_APPLY,1,32,13},
{Op::FLIGHT_PATH,1,32,13},
{Op::PARTICLE_FOLD,1,32,13},
{Op::FACE_FLUX_MOBILITY,1,32,13},
{Op::FLOW_ORIGIN_RESERVE,1,32,13},
{Op::FLOW_PAIR_APPLY,1,32,13},
{Op::FRICTION_REST_DEPOSIT,1,32,13},
{Op::ORDERED_CAPTURE,1,32,13},
{Op::MICRO_AUDIT_SEAL,1,32,13},
{Op::ACTIVATE_REQUEST,1,32,14},
{Op::ACTIVATE_APPLY,1,32,14},
{Op::FLIGHT_PATH,1,32,14},
{Op::PARTICLE_FOLD,1,32,14},
{Op::FACE_FLUX_MOBILITY,1,32,14},
{Op::FLOW_ORIGIN_RESERVE,1,32,14},
{Op::FLOW_PAIR_APPLY,1,32,14},
{Op::FRICTION_REST_DEPOSIT,1,32,14},
{Op::ORDERED_CAPTURE,1,32,14},
{Op::MICRO_AUDIT_SEAL,1,32,14},
{Op::ACTIVATE_REQUEST,1,32,15},
{Op::ACTIVATE_APPLY,1,32,15},
{Op::FLIGHT_PATH,1,32,15},
{Op::PARTICLE_FOLD,1,32,15},
{Op::FACE_FLUX_MOBILITY,1,32,15},
{Op::FLOW_ORIGIN_RESERVE,1,32,15},
{Op::FLOW_PAIR_APPLY,1,32,15},
{Op::FRICTION_REST_DEPOSIT,1,32,15},
{Op::ORDERED_CAPTURE,1,32,15},
{Op::MICRO_AUDIT_SEAL,1,32,15},
{Op::ACTIVATE_REQUEST,1,32,16},
{Op::ACTIVATE_APPLY,1,32,16},
{Op::FLIGHT_PATH,1,32,16},
{Op::PARTICLE_FOLD,1,32,16},
{Op::FACE_FLUX_MOBILITY,1,32,16},
{Op::FLOW_ORIGIN_RESERVE,1,32,16},
{Op::FLOW_PAIR_APPLY,1,32,16},
{Op::FRICTION_REST_DEPOSIT,1,32,16},
{Op::ORDERED_CAPTURE,1,32,16},
{Op::MICRO_AUDIT_SEAL,1,32,16},
{Op::ACTIVATE_REQUEST,1,32,17},
{Op::ACTIVATE_APPLY,1,32,17},
{Op::FLIGHT_PATH,1,32,17},
{Op::PARTICLE_FOLD,1,32,17},
{Op::FACE_FLUX_MOBILITY,1,32,17},
{Op::FLOW_ORIGIN_RESERVE,1,32,17},
{Op::FLOW_PAIR_APPLY,1,32,17},
{Op::FRICTION_REST_DEPOSIT,1,32,17},
{Op::ORDERED_CAPTURE,1,32,17},
{Op::MICRO_AUDIT_SEAL,1,32,17},
{Op::ACTIVATE_REQUEST,1,32,18},
{Op::ACTIVATE_APPLY,1,32,18},
{Op::FLIGHT_PATH,1,32,18},
{Op::PARTICLE_FOLD,1,32,18},
{Op::FACE_FLUX_MOBILITY,1,32,18},
{Op::FLOW_ORIGIN_RESERVE,1,32,18},
{Op::FLOW_PAIR_APPLY,1,32,18},
{Op::FRICTION_REST_DEPOSIT,1,32,18},
{Op::ORDERED_CAPTURE,1,32,18},
{Op::MICRO_AUDIT_SEAL,1,32,18},
{Op::ACTIVATE_REQUEST,1,32,19},
{Op::ACTIVATE_APPLY,1,32,19},
{Op::FLIGHT_PATH,1,32,19},
{Op::PARTICLE_FOLD,1,32,19},
{Op::FACE_FLUX_MOBILITY,1,32,19},
{Op::FLOW_ORIGIN_RESERVE,1,32,19},
{Op::FLOW_PAIR_APPLY,1,32,19},
{Op::FRICTION_REST_DEPOSIT,1,32,19},
{Op::ORDERED_CAPTURE,1,32,19},
{Op::MICRO_AUDIT_SEAL,1,32,19},
{Op::ACTIVATE_REQUEST,1,32,20},
{Op::ACTIVATE_APPLY,1,32,20},
{Op::FLIGHT_PATH,1,32,20},
{Op::PARTICLE_FOLD,1,32,20},
{Op::FACE_FLUX_MOBILITY,1,32,20},
{Op::FLOW_ORIGIN_RESERVE,1,32,20},
{Op::FLOW_PAIR_APPLY,1,32,20},
{Op::FRICTION_REST_DEPOSIT,1,32,20},
{Op::ORDERED_CAPTURE,1,32,20},
{Op::MICRO_AUDIT_SEAL,1,32,20},
{Op::ACTIVATE_REQUEST,1,32,21},
{Op::ACTIVATE_APPLY,1,32,21},
{Op::FLIGHT_PATH,1,32,21},
{Op::PARTICLE_FOLD,1,32,21},
{Op::FACE_FLUX_MOBILITY,1,32,21},
{Op::FLOW_ORIGIN_RESERVE,1,32,21},
{Op::FLOW_PAIR_APPLY,1,32,21},
{Op::FRICTION_REST_DEPOSIT,1,32,21},
{Op::ORDERED_CAPTURE,1,32,21},
{Op::MICRO_AUDIT_SEAL,1,32,21},
{Op::ACTIVATE_REQUEST,1,32,22},
{Op::ACTIVATE_APPLY,1,32,22},
{Op::FLIGHT_PATH,1,32,22},
{Op::PARTICLE_FOLD,1,32,22},
{Op::FACE_FLUX_MOBILITY,1,32,22},
{Op::FLOW_ORIGIN_RESERVE,1,32,22},
{Op::FLOW_PAIR_APPLY,1,32,22},
{Op::FRICTION_REST_DEPOSIT,1,32,22},
{Op::ORDERED_CAPTURE,1,32,22},
{Op::MICRO_AUDIT_SEAL,1,32,22},
{Op::ACTIVATE_REQUEST,1,32,23},
{Op::ACTIVATE_APPLY,1,32,23},
{Op::FLIGHT_PATH,1,32,23},
{Op::PARTICLE_FOLD,1,32,23},
{Op::FACE_FLUX_MOBILITY,1,32,23},
{Op::FLOW_ORIGIN_RESERVE,1,32,23},
{Op::FLOW_PAIR_APPLY,1,32,23},
{Op::FRICTION_REST_DEPOSIT,1,32,23},
{Op::ORDERED_CAPTURE,1,32,23},
{Op::MICRO_AUDIT_SEAL,1,32,23},
{Op::ACTIVATE_REQUEST,1,32,24},
{Op::ACTIVATE_APPLY,1,32,24},
{Op::FLIGHT_PATH,1,32,24},
{Op::PARTICLE_FOLD,1,32,24},
{Op::FACE_FLUX_MOBILITY,1,32,24},
{Op::FLOW_ORIGIN_RESERVE,1,32,24},
{Op::FLOW_PAIR_APPLY,1,32,24},
{Op::FRICTION_REST_DEPOSIT,1,32,24},
{Op::ORDERED_CAPTURE,1,32,24},
{Op::MICRO_AUDIT_SEAL,1,32,24},
{Op::ACTIVATE_REQUEST,1,32,25},
{Op::ACTIVATE_APPLY,1,32,25},
{Op::FLIGHT_PATH,1,32,25},
{Op::PARTICLE_FOLD,1,32,25},
{Op::FACE_FLUX_MOBILITY,1,32,25},
{Op::FLOW_ORIGIN_RESERVE,1,32,25},
{Op::FLOW_PAIR_APPLY,1,32,25},
{Op::FRICTION_REST_DEPOSIT,1,32,25},
{Op::ORDERED_CAPTURE,1,32,25},
{Op::MICRO_AUDIT_SEAL,1,32,25},
{Op::ACTIVATE_REQUEST,1,32,26},
{Op::ACTIVATE_APPLY,1,32,26},
{Op::FLIGHT_PATH,1,32,26},
{Op::PARTICLE_FOLD,1,32,26},
{Op::FACE_FLUX_MOBILITY,1,32,26},
{Op::FLOW_ORIGIN_RESERVE,1,32,26},
{Op::FLOW_PAIR_APPLY,1,32,26},
{Op::FRICTION_REST_DEPOSIT,1,32,26},
{Op::ORDERED_CAPTURE,1,32,26},
{Op::MICRO_AUDIT_SEAL,1,32,26},
{Op::ACTIVATE_REQUEST,1,32,27},
{Op::ACTIVATE_APPLY,1,32,27},
{Op::FLIGHT_PATH,1,32,27},
{Op::PARTICLE_FOLD,1,32,27},
{Op::FACE_FLUX_MOBILITY,1,32,27},
{Op::FLOW_ORIGIN_RESERVE,1,32,27},
{Op::FLOW_PAIR_APPLY,1,32,27},
{Op::FRICTION_REST_DEPOSIT,1,32,27},
{Op::ORDERED_CAPTURE,1,32,27},
{Op::MICRO_AUDIT_SEAL,1,32,27},
{Op::ACTIVATE_REQUEST,1,32,28},
{Op::ACTIVATE_APPLY,1,32,28},
{Op::FLIGHT_PATH,1,32,28},
{Op::PARTICLE_FOLD,1,32,28},
{Op::FACE_FLUX_MOBILITY,1,32,28},
{Op::FLOW_ORIGIN_RESERVE,1,32,28},
{Op::FLOW_PAIR_APPLY,1,32,28},
{Op::FRICTION_REST_DEPOSIT,1,32,28},
{Op::ORDERED_CAPTURE,1,32,28},
{Op::MICRO_AUDIT_SEAL,1,32,28},
{Op::ACTIVATE_REQUEST,1,32,29},
{Op::ACTIVATE_APPLY,1,32,29},
{Op::FLIGHT_PATH,1,32,29},
{Op::PARTICLE_FOLD,1,32,29},
{Op::FACE_FLUX_MOBILITY,1,32,29},
{Op::FLOW_ORIGIN_RESERVE,1,32,29},
{Op::FLOW_PAIR_APPLY,1,32,29},
{Op::FRICTION_REST_DEPOSIT,1,32,29},
{Op::ORDERED_CAPTURE,1,32,29},
{Op::MICRO_AUDIT_SEAL,1,32,29},
{Op::ACTIVATE_REQUEST,1,32,30},
{Op::ACTIVATE_APPLY,1,32,30},
{Op::FLIGHT_PATH,1,32,30},
{Op::PARTICLE_FOLD,1,32,30},
{Op::FACE_FLUX_MOBILITY,1,32,30},
{Op::FLOW_ORIGIN_RESERVE,1,32,30},
{Op::FLOW_PAIR_APPLY,1,32,30},
{Op::FRICTION_REST_DEPOSIT,1,32,30},
{Op::ORDERED_CAPTURE,1,32,30},
{Op::MICRO_AUDIT_SEAL,1,32,30},
{Op::ACTIVATE_REQUEST,1,32,31},
{Op::ACTIVATE_APPLY,1,32,31},
{Op::FLIGHT_PATH,1,32,31},
{Op::PARTICLE_FOLD,1,32,31},
{Op::FACE_FLUX_MOBILITY,1,32,31},
{Op::FLOW_ORIGIN_RESERVE,1,32,31},
{Op::FLOW_PAIR_APPLY,1,32,31},
{Op::FRICTION_REST_DEPOSIT,1,32,31},
{Op::ORDERED_CAPTURE,1,32,31},
{Op::MICRO_AUDIT_SEAL,1,32,31},
{Op::FINAL_MASS_ORIGIN_MOMENTUM,1,32,-1},
{Op::FINAL_ENERGY_CURSORS,1,32,-1},
{Op::SCOPE_SEAL_AND_CHOICE,1,32,-1},
{Op::DISCOVERY_COMMIT_SELECTION,0,0,-1},
{Op::AUTHORITATIVE_BEGIN,0,0,-1},
{Op::INIT_COPY,2,32,-1},
{Op::INPUT_AND_CFL,2,32,-1},
{Op::GRAVITY_WORK,2,32,-1},
{Op::TOUCH_ATOMIC,2,32,-1},
{Op::AFTER_TOUCH_CFL,2,32,-1},
{Op::ACTIVATE_REQUEST,2,32,0},
{Op::ACTIVATE_APPLY,2,32,0},
{Op::FLIGHT_PATH,2,32,0},
{Op::PARTICLE_FOLD,2,32,0},
{Op::FACE_FLUX_MOBILITY,2,32,0},
{Op::FLOW_ORIGIN_RESERVE,2,32,0},
{Op::FLOW_PAIR_APPLY,2,32,0},
{Op::FRICTION_REST_DEPOSIT,2,32,0},
{Op::ORDERED_CAPTURE,2,32,0},
{Op::MICRO_AUDIT_SEAL,2,32,0},
{Op::ACTIVATE_REQUEST,2,32,1},
{Op::ACTIVATE_APPLY,2,32,1},
{Op::FLIGHT_PATH,2,32,1},
{Op::PARTICLE_FOLD,2,32,1},
{Op::FACE_FLUX_MOBILITY,2,32,1},
{Op::FLOW_ORIGIN_RESERVE,2,32,1},
{Op::FLOW_PAIR_APPLY,2,32,1},
{Op::FRICTION_REST_DEPOSIT,2,32,1},
{Op::ORDERED_CAPTURE,2,32,1},
{Op::MICRO_AUDIT_SEAL,2,32,1},
{Op::ACTIVATE_REQUEST,2,32,2},
{Op::ACTIVATE_APPLY,2,32,2},
{Op::FLIGHT_PATH,2,32,2},
{Op::PARTICLE_FOLD,2,32,2},
{Op::FACE_FLUX_MOBILITY,2,32,2},
{Op::FLOW_ORIGIN_RESERVE,2,32,2},
{Op::FLOW_PAIR_APPLY,2,32,2},
{Op::FRICTION_REST_DEPOSIT,2,32,2},
{Op::ORDERED_CAPTURE,2,32,2},
{Op::MICRO_AUDIT_SEAL,2,32,2},
{Op::ACTIVATE_REQUEST,2,32,3},
{Op::ACTIVATE_APPLY,2,32,3},
{Op::FLIGHT_PATH,2,32,3},
{Op::PARTICLE_FOLD,2,32,3},
{Op::FACE_FLUX_MOBILITY,2,32,3},
{Op::FLOW_ORIGIN_RESERVE,2,32,3},
{Op::FLOW_PAIR_APPLY,2,32,3},
{Op::FRICTION_REST_DEPOSIT,2,32,3},
{Op::ORDERED_CAPTURE,2,32,3},
{Op::MICRO_AUDIT_SEAL,2,32,3},
{Op::ACTIVATE_REQUEST,2,32,4},
{Op::ACTIVATE_APPLY,2,32,4},
{Op::FLIGHT_PATH,2,32,4},
{Op::PARTICLE_FOLD,2,32,4},
{Op::FACE_FLUX_MOBILITY,2,32,4},
{Op::FLOW_ORIGIN_RESERVE,2,32,4},
{Op::FLOW_PAIR_APPLY,2,32,4},
{Op::FRICTION_REST_DEPOSIT,2,32,4},
{Op::ORDERED_CAPTURE,2,32,4},
{Op::MICRO_AUDIT_SEAL,2,32,4},
{Op::ACTIVATE_REQUEST,2,32,5},
{Op::ACTIVATE_APPLY,2,32,5},
{Op::FLIGHT_PATH,2,32,5},
{Op::PARTICLE_FOLD,2,32,5},
{Op::FACE_FLUX_MOBILITY,2,32,5},
{Op::FLOW_ORIGIN_RESERVE,2,32,5},
{Op::FLOW_PAIR_APPLY,2,32,5},
{Op::FRICTION_REST_DEPOSIT,2,32,5},
{Op::ORDERED_CAPTURE,2,32,5},
{Op::MICRO_AUDIT_SEAL,2,32,5},
{Op::ACTIVATE_REQUEST,2,32,6},
{Op::ACTIVATE_APPLY,2,32,6},
{Op::FLIGHT_PATH,2,32,6},
{Op::PARTICLE_FOLD,2,32,6},
{Op::FACE_FLUX_MOBILITY,2,32,6},
{Op::FLOW_ORIGIN_RESERVE,2,32,6},
{Op::FLOW_PAIR_APPLY,2,32,6},
{Op::FRICTION_REST_DEPOSIT,2,32,6},
{Op::ORDERED_CAPTURE,2,32,6},
{Op::MICRO_AUDIT_SEAL,2,32,6},
{Op::ACTIVATE_REQUEST,2,32,7},
{Op::ACTIVATE_APPLY,2,32,7},
{Op::FLIGHT_PATH,2,32,7},
{Op::PARTICLE_FOLD,2,32,7},
{Op::FACE_FLUX_MOBILITY,2,32,7},
{Op::FLOW_ORIGIN_RESERVE,2,32,7},
{Op::FLOW_PAIR_APPLY,2,32,7},
{Op::FRICTION_REST_DEPOSIT,2,32,7},
{Op::ORDERED_CAPTURE,2,32,7},
{Op::MICRO_AUDIT_SEAL,2,32,7},
{Op::ACTIVATE_REQUEST,2,32,8},
{Op::ACTIVATE_APPLY,2,32,8},
{Op::FLIGHT_PATH,2,32,8},
{Op::PARTICLE_FOLD,2,32,8},
{Op::FACE_FLUX_MOBILITY,2,32,8},
{Op::FLOW_ORIGIN_RESERVE,2,32,8},
{Op::FLOW_PAIR_APPLY,2,32,8},
{Op::FRICTION_REST_DEPOSIT,2,32,8},
{Op::ORDERED_CAPTURE,2,32,8},
{Op::MICRO_AUDIT_SEAL,2,32,8},
{Op::ACTIVATE_REQUEST,2,32,9},
{Op::ACTIVATE_APPLY,2,32,9},
{Op::FLIGHT_PATH,2,32,9},
{Op::PARTICLE_FOLD,2,32,9},
{Op::FACE_FLUX_MOBILITY,2,32,9},
{Op::FLOW_ORIGIN_RESERVE,2,32,9},
{Op::FLOW_PAIR_APPLY,2,32,9},
{Op::FRICTION_REST_DEPOSIT,2,32,9},
{Op::ORDERED_CAPTURE,2,32,9},
{Op::MICRO_AUDIT_SEAL,2,32,9},
{Op::ACTIVATE_REQUEST,2,32,10},
{Op::ACTIVATE_APPLY,2,32,10},
{Op::FLIGHT_PATH,2,32,10},
{Op::PARTICLE_FOLD,2,32,10},
{Op::FACE_FLUX_MOBILITY,2,32,10},
{Op::FLOW_ORIGIN_RESERVE,2,32,10},
{Op::FLOW_PAIR_APPLY,2,32,10},
{Op::FRICTION_REST_DEPOSIT,2,32,10},
{Op::ORDERED_CAPTURE,2,32,10},
{Op::MICRO_AUDIT_SEAL,2,32,10},
{Op::ACTIVATE_REQUEST,2,32,11},
{Op::ACTIVATE_APPLY,2,32,11},
{Op::FLIGHT_PATH,2,32,11},
{Op::PARTICLE_FOLD,2,32,11},
{Op::FACE_FLUX_MOBILITY,2,32,11},
{Op::FLOW_ORIGIN_RESERVE,2,32,11},
{Op::FLOW_PAIR_APPLY,2,32,11},
{Op::FRICTION_REST_DEPOSIT,2,32,11},
{Op::ORDERED_CAPTURE,2,32,11},
{Op::MICRO_AUDIT_SEAL,2,32,11},
{Op::ACTIVATE_REQUEST,2,32,12},
{Op::ACTIVATE_APPLY,2,32,12},
{Op::FLIGHT_PATH,2,32,12},
{Op::PARTICLE_FOLD,2,32,12},
{Op::FACE_FLUX_MOBILITY,2,32,12},
{Op::FLOW_ORIGIN_RESERVE,2,32,12},
{Op::FLOW_PAIR_APPLY,2,32,12},
{Op::FRICTION_REST_DEPOSIT,2,32,12},
{Op::ORDERED_CAPTURE,2,32,12},
{Op::MICRO_AUDIT_SEAL,2,32,12},
{Op::ACTIVATE_REQUEST,2,32,13},
{Op::ACTIVATE_APPLY,2,32,13},
{Op::FLIGHT_PATH,2,32,13},
{Op::PARTICLE_FOLD,2,32,13},
{Op::FACE_FLUX_MOBILITY,2,32,13},
{Op::FLOW_ORIGIN_RESERVE,2,32,13},
{Op::FLOW_PAIR_APPLY,2,32,13},
{Op::FRICTION_REST_DEPOSIT,2,32,13},
{Op::ORDERED_CAPTURE,2,32,13},
{Op::MICRO_AUDIT_SEAL,2,32,13},
{Op::ACTIVATE_REQUEST,2,32,14},
{Op::ACTIVATE_APPLY,2,32,14},
{Op::FLIGHT_PATH,2,32,14},
{Op::PARTICLE_FOLD,2,32,14},
{Op::FACE_FLUX_MOBILITY,2,32,14},
{Op::FLOW_ORIGIN_RESERVE,2,32,14},
{Op::FLOW_PAIR_APPLY,2,32,14},
{Op::FRICTION_REST_DEPOSIT,2,32,14},
{Op::ORDERED_CAPTURE,2,32,14},
{Op::MICRO_AUDIT_SEAL,2,32,14},
{Op::ACTIVATE_REQUEST,2,32,15},
{Op::ACTIVATE_APPLY,2,32,15},
{Op::FLIGHT_PATH,2,32,15},
{Op::PARTICLE_FOLD,2,32,15},
{Op::FACE_FLUX_MOBILITY,2,32,15},
{Op::FLOW_ORIGIN_RESERVE,2,32,15},
{Op::FLOW_PAIR_APPLY,2,32,15},
{Op::FRICTION_REST_DEPOSIT,2,32,15},
{Op::ORDERED_CAPTURE,2,32,15},
{Op::MICRO_AUDIT_SEAL,2,32,15},
{Op::ACTIVATE_REQUEST,2,32,16},
{Op::ACTIVATE_APPLY,2,32,16},
{Op::FLIGHT_PATH,2,32,16},
{Op::PARTICLE_FOLD,2,32,16},
{Op::FACE_FLUX_MOBILITY,2,32,16},
{Op::FLOW_ORIGIN_RESERVE,2,32,16},
{Op::FLOW_PAIR_APPLY,2,32,16},
{Op::FRICTION_REST_DEPOSIT,2,32,16},
{Op::ORDERED_CAPTURE,2,32,16},
{Op::MICRO_AUDIT_SEAL,2,32,16},
{Op::ACTIVATE_REQUEST,2,32,17},
{Op::ACTIVATE_APPLY,2,32,17},
{Op::FLIGHT_PATH,2,32,17},
{Op::PARTICLE_FOLD,2,32,17},
{Op::FACE_FLUX_MOBILITY,2,32,17},
{Op::FLOW_ORIGIN_RESERVE,2,32,17},
{Op::FLOW_PAIR_APPLY,2,32,17},
{Op::FRICTION_REST_DEPOSIT,2,32,17},
{Op::ORDERED_CAPTURE,2,32,17},
{Op::MICRO_AUDIT_SEAL,2,32,17},
{Op::ACTIVATE_REQUEST,2,32,18},
{Op::ACTIVATE_APPLY,2,32,18},
{Op::FLIGHT_PATH,2,32,18},
{Op::PARTICLE_FOLD,2,32,18},
{Op::FACE_FLUX_MOBILITY,2,32,18},
{Op::FLOW_ORIGIN_RESERVE,2,32,18},
{Op::FLOW_PAIR_APPLY,2,32,18},
{Op::FRICTION_REST_DEPOSIT,2,32,18},
{Op::ORDERED_CAPTURE,2,32,18},
{Op::MICRO_AUDIT_SEAL,2,32,18},
{Op::ACTIVATE_REQUEST,2,32,19},
{Op::ACTIVATE_APPLY,2,32,19},
{Op::FLIGHT_PATH,2,32,19},
{Op::PARTICLE_FOLD,2,32,19},
{Op::FACE_FLUX_MOBILITY,2,32,19},
{Op::FLOW_ORIGIN_RESERVE,2,32,19},
{Op::FLOW_PAIR_APPLY,2,32,19},
{Op::FRICTION_REST_DEPOSIT,2,32,19},
{Op::ORDERED_CAPTURE,2,32,19},
{Op::MICRO_AUDIT_SEAL,2,32,19},
{Op::ACTIVATE_REQUEST,2,32,20},
{Op::ACTIVATE_APPLY,2,32,20},
{Op::FLIGHT_PATH,2,32,20},
{Op::PARTICLE_FOLD,2,32,20},
{Op::FACE_FLUX_MOBILITY,2,32,20},
{Op::FLOW_ORIGIN_RESERVE,2,32,20},
{Op::FLOW_PAIR_APPLY,2,32,20},
{Op::FRICTION_REST_DEPOSIT,2,32,20},
{Op::ORDERED_CAPTURE,2,32,20},
{Op::MICRO_AUDIT_SEAL,2,32,20},
{Op::ACTIVATE_REQUEST,2,32,21},
{Op::ACTIVATE_APPLY,2,32,21},
{Op::FLIGHT_PATH,2,32,21},
{Op::PARTICLE_FOLD,2,32,21},
{Op::FACE_FLUX_MOBILITY,2,32,21},
{Op::FLOW_ORIGIN_RESERVE,2,32,21},
{Op::FLOW_PAIR_APPLY,2,32,21},
{Op::FRICTION_REST_DEPOSIT,2,32,21},
{Op::ORDERED_CAPTURE,2,32,21},
{Op::MICRO_AUDIT_SEAL,2,32,21},
{Op::ACTIVATE_REQUEST,2,32,22},
{Op::ACTIVATE_APPLY,2,32,22},
{Op::FLIGHT_PATH,2,32,22},
{Op::PARTICLE_FOLD,2,32,22},
{Op::FACE_FLUX_MOBILITY,2,32,22},
{Op::FLOW_ORIGIN_RESERVE,2,32,22},
{Op::FLOW_PAIR_APPLY,2,32,22},
{Op::FRICTION_REST_DEPOSIT,2,32,22},
{Op::ORDERED_CAPTURE,2,32,22},
{Op::MICRO_AUDIT_SEAL,2,32,22},
{Op::ACTIVATE_REQUEST,2,32,23},
{Op::ACTIVATE_APPLY,2,32,23},
{Op::FLIGHT_PATH,2,32,23},
{Op::PARTICLE_FOLD,2,32,23},
{Op::FACE_FLUX_MOBILITY,2,32,23},
{Op::FLOW_ORIGIN_RESERVE,2,32,23},
{Op::FLOW_PAIR_APPLY,2,32,23},
{Op::FRICTION_REST_DEPOSIT,2,32,23},
{Op::ORDERED_CAPTURE,2,32,23},
{Op::MICRO_AUDIT_SEAL,2,32,23},
{Op::ACTIVATE_REQUEST,2,32,24},
{Op::ACTIVATE_APPLY,2,32,24},
{Op::FLIGHT_PATH,2,32,24},
{Op::PARTICLE_FOLD,2,32,24},
{Op::FACE_FLUX_MOBILITY,2,32,24},
{Op::FLOW_ORIGIN_RESERVE,2,32,24},
{Op::FLOW_PAIR_APPLY,2,32,24},
{Op::FRICTION_REST_DEPOSIT,2,32,24},
{Op::ORDERED_CAPTURE,2,32,24},
{Op::MICRO_AUDIT_SEAL,2,32,24},
{Op::ACTIVATE_REQUEST,2,32,25},
{Op::ACTIVATE_APPLY,2,32,25},
{Op::FLIGHT_PATH,2,32,25},
{Op::PARTICLE_FOLD,2,32,25},
{Op::FACE_FLUX_MOBILITY,2,32,25},
{Op::FLOW_ORIGIN_RESERVE,2,32,25},
{Op::FLOW_PAIR_APPLY,2,32,25},
{Op::FRICTION_REST_DEPOSIT,2,32,25},
{Op::ORDERED_CAPTURE,2,32,25},
{Op::MICRO_AUDIT_SEAL,2,32,25},
{Op::ACTIVATE_REQUEST,2,32,26},
{Op::ACTIVATE_APPLY,2,32,26},
{Op::FLIGHT_PATH,2,32,26},
{Op::PARTICLE_FOLD,2,32,26},
{Op::FACE_FLUX_MOBILITY,2,32,26},
{Op::FLOW_ORIGIN_RESERVE,2,32,26},
{Op::FLOW_PAIR_APPLY,2,32,26},
{Op::FRICTION_REST_DEPOSIT,2,32,26},
{Op::ORDERED_CAPTURE,2,32,26},
{Op::MICRO_AUDIT_SEAL,2,32,26},
{Op::ACTIVATE_REQUEST,2,32,27},
{Op::ACTIVATE_APPLY,2,32,27},
{Op::FLIGHT_PATH,2,32,27},
{Op::PARTICLE_FOLD,2,32,27},
{Op::FACE_FLUX_MOBILITY,2,32,27},
{Op::FLOW_ORIGIN_RESERVE,2,32,27},
{Op::FLOW_PAIR_APPLY,2,32,27},
{Op::FRICTION_REST_DEPOSIT,2,32,27},
{Op::ORDERED_CAPTURE,2,32,27},
{Op::MICRO_AUDIT_SEAL,2,32,27},
{Op::ACTIVATE_REQUEST,2,32,28},
{Op::ACTIVATE_APPLY,2,32,28},
{Op::FLIGHT_PATH,2,32,28},
{Op::PARTICLE_FOLD,2,32,28},
{Op::FACE_FLUX_MOBILITY,2,32,28},
{Op::FLOW_ORIGIN_RESERVE,2,32,28},
{Op::FLOW_PAIR_APPLY,2,32,28},
{Op::FRICTION_REST_DEPOSIT,2,32,28},
{Op::ORDERED_CAPTURE,2,32,28},
{Op::MICRO_AUDIT_SEAL,2,32,28},
{Op::ACTIVATE_REQUEST,2,32,29},
{Op::ACTIVATE_APPLY,2,32,29},
{Op::FLIGHT_PATH,2,32,29},
{Op::PARTICLE_FOLD,2,32,29},
{Op::FACE_FLUX_MOBILITY,2,32,29},
{Op::FLOW_ORIGIN_RESERVE,2,32,29},
{Op::FLOW_PAIR_APPLY,2,32,29},
{Op::FRICTION_REST_DEPOSIT,2,32,29},
{Op::ORDERED_CAPTURE,2,32,29},
{Op::MICRO_AUDIT_SEAL,2,32,29},
{Op::ACTIVATE_REQUEST,2,32,30},
{Op::ACTIVATE_APPLY,2,32,30},
{Op::FLIGHT_PATH,2,32,30},
{Op::PARTICLE_FOLD,2,32,30},
{Op::FACE_FLUX_MOBILITY,2,32,30},
{Op::FLOW_ORIGIN_RESERVE,2,32,30},
{Op::FLOW_PAIR_APPLY,2,32,30},
{Op::FRICTION_REST_DEPOSIT,2,32,30},
{Op::ORDERED_CAPTURE,2,32,30},
{Op::MICRO_AUDIT_SEAL,2,32,30},
{Op::ACTIVATE_REQUEST,2,32,31},
{Op::ACTIVATE_APPLY,2,32,31},
{Op::FLIGHT_PATH,2,32,31},
{Op::PARTICLE_FOLD,2,32,31},
{Op::FACE_FLUX_MOBILITY,2,32,31},
{Op::FLOW_ORIGIN_RESERVE,2,32,31},
{Op::FLOW_PAIR_APPLY,2,32,31},
{Op::FRICTION_REST_DEPOSIT,2,32,31},
{Op::ORDERED_CAPTURE,2,32,31},
{Op::MICRO_AUDIT_SEAL,2,32,31},
{Op::FINAL_MASS_ORIGIN_MOMENTUM,2,32,-1},
{Op::FINAL_ENERGY_CURSORS,2,32,-1},
{Op::SCOPE_SEAL_AND_CHOICE,2,32,-1},
{Op::REJECT_PREFIX_BEGIN,0,0,-1},
{Op::INIT_COPY,3,1,-1},
{Op::INPUT_AND_CFL,3,1,-1},
{Op::GRAVITY_WORK,3,1,-1},
{Op::TOUCH_ATOMIC,3,1,-1},
{Op::AFTER_TOUCH_CFL,3,1,-1},
{Op::ACTIVATE_REQUEST,3,1,0},
{Op::ACTIVATE_APPLY,3,1,0},
{Op::FLIGHT_PATH,3,1,0},
{Op::PARTICLE_FOLD,3,1,0},
{Op::FACE_FLUX_MOBILITY,3,1,0},
{Op::FLOW_ORIGIN_RESERVE,3,1,0},
{Op::FLOW_PAIR_APPLY,3,1,0},
{Op::FRICTION_REST_DEPOSIT,3,1,0},
{Op::ORDERED_CAPTURE,3,1,0},
{Op::MICRO_AUDIT_SEAL,3,1,0},
{Op::FINAL_MASS_ORIGIN_MOMENTUM,3,1,-1},
{Op::FINAL_ENERGY_CURSORS,3,1,-1},
{Op::SCOPE_SEAL_AND_CHOICE,3,1,-1},
{Op::INIT_COPY,3,2,-1},
{Op::INPUT_AND_CFL,3,2,-1},
{Op::GRAVITY_WORK,3,2,-1},
{Op::TOUCH_ATOMIC,3,2,-1},
{Op::AFTER_TOUCH_CFL,3,2,-1},
{Op::ACTIVATE_REQUEST,3,2,0},
{Op::ACTIVATE_APPLY,3,2,0},
{Op::FLIGHT_PATH,3,2,0},
{Op::PARTICLE_FOLD,3,2,0},
{Op::FACE_FLUX_MOBILITY,3,2,0},
{Op::FLOW_ORIGIN_RESERVE,3,2,0},
{Op::FLOW_PAIR_APPLY,3,2,0},
{Op::FRICTION_REST_DEPOSIT,3,2,0},
{Op::ORDERED_CAPTURE,3,2,0},
{Op::MICRO_AUDIT_SEAL,3,2,0},
{Op::ACTIVATE_REQUEST,3,2,1},
{Op::ACTIVATE_APPLY,3,2,1},
{Op::FLIGHT_PATH,3,2,1},
{Op::PARTICLE_FOLD,3,2,1},
{Op::FACE_FLUX_MOBILITY,3,2,1},
{Op::FLOW_ORIGIN_RESERVE,3,2,1},
{Op::FLOW_PAIR_APPLY,3,2,1},
{Op::FRICTION_REST_DEPOSIT,3,2,1},
{Op::ORDERED_CAPTURE,3,2,1},
{Op::MICRO_AUDIT_SEAL,3,2,1},
{Op::FINAL_MASS_ORIGIN_MOMENTUM,3,2,-1},
{Op::FINAL_ENERGY_CURSORS,3,2,-1},
{Op::SCOPE_SEAL_AND_CHOICE,3,2,-1},
{Op::INIT_COPY,3,4,-1},
{Op::INPUT_AND_CFL,3,4,-1},
{Op::GRAVITY_WORK,3,4,-1},
{Op::TOUCH_ATOMIC,3,4,-1},
{Op::AFTER_TOUCH_CFL,3,4,-1},
{Op::ACTIVATE_REQUEST,3,4,0},
{Op::ACTIVATE_APPLY,3,4,0},
{Op::FLIGHT_PATH,3,4,0},
{Op::PARTICLE_FOLD,3,4,0},
{Op::FACE_FLUX_MOBILITY,3,4,0},
{Op::FLOW_ORIGIN_RESERVE,3,4,0},
{Op::FLOW_PAIR_APPLY,3,4,0},
{Op::FRICTION_REST_DEPOSIT,3,4,0},
{Op::ORDERED_CAPTURE,3,4,0},
{Op::MICRO_AUDIT_SEAL,3,4,0},
{Op::ACTIVATE_REQUEST,3,4,1},
{Op::ACTIVATE_APPLY,3,4,1},
{Op::FLIGHT_PATH,3,4,1},
{Op::PARTICLE_FOLD,3,4,1},
{Op::FACE_FLUX_MOBILITY,3,4,1},
{Op::FLOW_ORIGIN_RESERVE,3,4,1},
{Op::FLOW_PAIR_APPLY,3,4,1},
{Op::FRICTION_REST_DEPOSIT,3,4,1},
{Op::ORDERED_CAPTURE,3,4,1},
{Op::MICRO_AUDIT_SEAL,3,4,1},
{Op::ACTIVATE_REQUEST,3,4,2},
{Op::ACTIVATE_APPLY,3,4,2},
{Op::FLIGHT_PATH,3,4,2},
{Op::PARTICLE_FOLD,3,4,2},
{Op::FACE_FLUX_MOBILITY,3,4,2},
{Op::FLOW_ORIGIN_RESERVE,3,4,2},
{Op::FLOW_PAIR_APPLY,3,4,2},
{Op::FRICTION_REST_DEPOSIT,3,4,2},
{Op::ORDERED_CAPTURE,3,4,2},
{Op::MICRO_AUDIT_SEAL,3,4,2},
{Op::ACTIVATE_REQUEST,3,4,3},
{Op::ACTIVATE_APPLY,3,4,3},
{Op::FLIGHT_PATH,3,4,3},
{Op::PARTICLE_FOLD,3,4,3},
{Op::FACE_FLUX_MOBILITY,3,4,3},
{Op::FLOW_ORIGIN_RESERVE,3,4,3},
{Op::FLOW_PAIR_APPLY,3,4,3},
{Op::FRICTION_REST_DEPOSIT,3,4,3},
{Op::ORDERED_CAPTURE,3,4,3},
{Op::MICRO_AUDIT_SEAL,3,4,3},
{Op::FINAL_MASS_ORIGIN_MOMENTUM,3,4,-1},
{Op::FINAL_ENERGY_CURSORS,3,4,-1},
{Op::SCOPE_SEAL_AND_CHOICE,3,4,-1},
{Op::INIT_COPY,3,8,-1},
{Op::INPUT_AND_CFL,3,8,-1},
{Op::GRAVITY_WORK,3,8,-1},
{Op::TOUCH_ATOMIC,3,8,-1},
{Op::AFTER_TOUCH_CFL,3,8,-1},
{Op::ACTIVATE_REQUEST,3,8,0},
{Op::ACTIVATE_APPLY,3,8,0},
{Op::FLIGHT_PATH,3,8,0},
{Op::PARTICLE_FOLD,3,8,0},
{Op::FACE_FLUX_MOBILITY,3,8,0},
{Op::FLOW_ORIGIN_RESERVE,3,8,0},
{Op::FLOW_PAIR_APPLY,3,8,0},
{Op::FRICTION_REST_DEPOSIT,3,8,0},
{Op::ORDERED_CAPTURE,3,8,0},
{Op::MICRO_AUDIT_SEAL,3,8,0},
{Op::ACTIVATE_REQUEST,3,8,1},
{Op::ACTIVATE_APPLY,3,8,1},
{Op::FLIGHT_PATH,3,8,1},
{Op::PARTICLE_FOLD,3,8,1},
{Op::FACE_FLUX_MOBILITY,3,8,1},
{Op::FLOW_ORIGIN_RESERVE,3,8,1},
{Op::FLOW_PAIR_APPLY,3,8,1},
{Op::FRICTION_REST_DEPOSIT,3,8,1},
{Op::ORDERED_CAPTURE,3,8,1},
{Op::MICRO_AUDIT_SEAL,3,8,1},
{Op::ACTIVATE_REQUEST,3,8,2},
{Op::ACTIVATE_APPLY,3,8,2},
{Op::FLIGHT_PATH,3,8,2},
{Op::PARTICLE_FOLD,3,8,2},
{Op::FACE_FLUX_MOBILITY,3,8,2},
{Op::FLOW_ORIGIN_RESERVE,3,8,2},
{Op::FLOW_PAIR_APPLY,3,8,2},
{Op::FRICTION_REST_DEPOSIT,3,8,2},
{Op::ORDERED_CAPTURE,3,8,2},
{Op::MICRO_AUDIT_SEAL,3,8,2},
{Op::ACTIVATE_REQUEST,3,8,3},
{Op::ACTIVATE_APPLY,3,8,3},
{Op::FLIGHT_PATH,3,8,3},
{Op::PARTICLE_FOLD,3,8,3},
{Op::FACE_FLUX_MOBILITY,3,8,3},
{Op::FLOW_ORIGIN_RESERVE,3,8,3},
{Op::FLOW_PAIR_APPLY,3,8,3},
{Op::FRICTION_REST_DEPOSIT,3,8,3},
{Op::ORDERED_CAPTURE,3,8,3},
{Op::MICRO_AUDIT_SEAL,3,8,3},
{Op::ACTIVATE_REQUEST,3,8,4},
{Op::ACTIVATE_APPLY,3,8,4},
{Op::FLIGHT_PATH,3,8,4},
{Op::PARTICLE_FOLD,3,8,4},
{Op::FACE_FLUX_MOBILITY,3,8,4},
{Op::FLOW_ORIGIN_RESERVE,3,8,4},
{Op::FLOW_PAIR_APPLY,3,8,4},
{Op::FRICTION_REST_DEPOSIT,3,8,4},
{Op::ORDERED_CAPTURE,3,8,4},
{Op::MICRO_AUDIT_SEAL,3,8,4},
{Op::ACTIVATE_REQUEST,3,8,5},
{Op::ACTIVATE_APPLY,3,8,5},
{Op::FLIGHT_PATH,3,8,5},
{Op::PARTICLE_FOLD,3,8,5},
{Op::FACE_FLUX_MOBILITY,3,8,5},
{Op::FLOW_ORIGIN_RESERVE,3,8,5},
{Op::FLOW_PAIR_APPLY,3,8,5},
{Op::FRICTION_REST_DEPOSIT,3,8,5},
{Op::ORDERED_CAPTURE,3,8,5},
{Op::MICRO_AUDIT_SEAL,3,8,5},
{Op::ACTIVATE_REQUEST,3,8,6},
{Op::ACTIVATE_APPLY,3,8,6},
{Op::FLIGHT_PATH,3,8,6},
{Op::PARTICLE_FOLD,3,8,6},
{Op::FACE_FLUX_MOBILITY,3,8,6},
{Op::FLOW_ORIGIN_RESERVE,3,8,6},
{Op::FLOW_PAIR_APPLY,3,8,6},
{Op::FRICTION_REST_DEPOSIT,3,8,6},
{Op::ORDERED_CAPTURE,3,8,6},
{Op::MICRO_AUDIT_SEAL,3,8,6},
{Op::ACTIVATE_REQUEST,3,8,7},
{Op::ACTIVATE_APPLY,3,8,7},
{Op::FLIGHT_PATH,3,8,7},
{Op::PARTICLE_FOLD,3,8,7},
{Op::FACE_FLUX_MOBILITY,3,8,7},
{Op::FLOW_ORIGIN_RESERVE,3,8,7},
{Op::FLOW_PAIR_APPLY,3,8,7},
{Op::FRICTION_REST_DEPOSIT,3,8,7},
{Op::ORDERED_CAPTURE,3,8,7},
{Op::MICRO_AUDIT_SEAL,3,8,7},
{Op::FINAL_MASS_ORIGIN_MOMENTUM,3,8,-1},
{Op::FINAL_ENERGY_CURSORS,3,8,-1},
{Op::SCOPE_SEAL_AND_CHOICE,3,8,-1},
{Op::INIT_COPY,3,16,-1},
{Op::INPUT_AND_CFL,3,16,-1},
{Op::GRAVITY_WORK,3,16,-1},
{Op::TOUCH_ATOMIC,3,16,-1},
{Op::AFTER_TOUCH_CFL,3,16,-1},
{Op::ACTIVATE_REQUEST,3,16,0},
{Op::ACTIVATE_APPLY,3,16,0},
{Op::FLIGHT_PATH,3,16,0},
{Op::PARTICLE_FOLD,3,16,0},
{Op::FACE_FLUX_MOBILITY,3,16,0},
{Op::FLOW_ORIGIN_RESERVE,3,16,0},
{Op::FLOW_PAIR_APPLY,3,16,0},
{Op::FRICTION_REST_DEPOSIT,3,16,0},
{Op::ORDERED_CAPTURE,3,16,0},
{Op::MICRO_AUDIT_SEAL,3,16,0},
{Op::ACTIVATE_REQUEST,3,16,1},
{Op::ACTIVATE_APPLY,3,16,1},
{Op::FLIGHT_PATH,3,16,1},
{Op::PARTICLE_FOLD,3,16,1},
{Op::FACE_FLUX_MOBILITY,3,16,1},
{Op::FLOW_ORIGIN_RESERVE,3,16,1},
{Op::FLOW_PAIR_APPLY,3,16,1},
{Op::FRICTION_REST_DEPOSIT,3,16,1},
{Op::ORDERED_CAPTURE,3,16,1},
{Op::MICRO_AUDIT_SEAL,3,16,1},
{Op::ACTIVATE_REQUEST,3,16,2},
{Op::ACTIVATE_APPLY,3,16,2},
{Op::FLIGHT_PATH,3,16,2},
{Op::PARTICLE_FOLD,3,16,2},
{Op::FACE_FLUX_MOBILITY,3,16,2},
{Op::FLOW_ORIGIN_RESERVE,3,16,2},
{Op::FLOW_PAIR_APPLY,3,16,2},
{Op::FRICTION_REST_DEPOSIT,3,16,2},
{Op::ORDERED_CAPTURE,3,16,2},
{Op::MICRO_AUDIT_SEAL,3,16,2},
{Op::ACTIVATE_REQUEST,3,16,3},
{Op::ACTIVATE_APPLY,3,16,3},
{Op::FLIGHT_PATH,3,16,3},
{Op::PARTICLE_FOLD,3,16,3},
{Op::FACE_FLUX_MOBILITY,3,16,3},
{Op::FLOW_ORIGIN_RESERVE,3,16,3},
{Op::FLOW_PAIR_APPLY,3,16,3},
{Op::FRICTION_REST_DEPOSIT,3,16,3},
{Op::ORDERED_CAPTURE,3,16,3},
{Op::MICRO_AUDIT_SEAL,3,16,3},
{Op::ACTIVATE_REQUEST,3,16,4},
{Op::ACTIVATE_APPLY,3,16,4},
{Op::FLIGHT_PATH,3,16,4},
{Op::PARTICLE_FOLD,3,16,4},
{Op::FACE_FLUX_MOBILITY,3,16,4},
{Op::FLOW_ORIGIN_RESERVE,3,16,4},
{Op::FLOW_PAIR_APPLY,3,16,4},
{Op::FRICTION_REST_DEPOSIT,3,16,4},
{Op::ORDERED_CAPTURE,3,16,4},
{Op::MICRO_AUDIT_SEAL,3,16,4},
{Op::ACTIVATE_REQUEST,3,16,5},
{Op::ACTIVATE_APPLY,3,16,5},
{Op::FLIGHT_PATH,3,16,5},
{Op::PARTICLE_FOLD,3,16,5},
{Op::FACE_FLUX_MOBILITY,3,16,5},
{Op::FLOW_ORIGIN_RESERVE,3,16,5},
{Op::FLOW_PAIR_APPLY,3,16,5},
{Op::FRICTION_REST_DEPOSIT,3,16,5},
{Op::ORDERED_CAPTURE,3,16,5},
{Op::MICRO_AUDIT_SEAL,3,16,5},
{Op::ACTIVATE_REQUEST,3,16,6},
{Op::ACTIVATE_APPLY,3,16,6},
{Op::FLIGHT_PATH,3,16,6},
{Op::PARTICLE_FOLD,3,16,6},
{Op::FACE_FLUX_MOBILITY,3,16,6},
{Op::FLOW_ORIGIN_RESERVE,3,16,6},
{Op::FLOW_PAIR_APPLY,3,16,6},
{Op::FRICTION_REST_DEPOSIT,3,16,6},
{Op::ORDERED_CAPTURE,3,16,6},
{Op::MICRO_AUDIT_SEAL,3,16,6},
{Op::ACTIVATE_REQUEST,3,16,7},
{Op::ACTIVATE_APPLY,3,16,7},
{Op::FLIGHT_PATH,3,16,7},
{Op::PARTICLE_FOLD,3,16,7},
{Op::FACE_FLUX_MOBILITY,3,16,7},
{Op::FLOW_ORIGIN_RESERVE,3,16,7},
{Op::FLOW_PAIR_APPLY,3,16,7},
{Op::FRICTION_REST_DEPOSIT,3,16,7},
{Op::ORDERED_CAPTURE,3,16,7},
{Op::MICRO_AUDIT_SEAL,3,16,7},
{Op::ACTIVATE_REQUEST,3,16,8},
{Op::ACTIVATE_APPLY,3,16,8},
{Op::FLIGHT_PATH,3,16,8},
{Op::PARTICLE_FOLD,3,16,8},
{Op::FACE_FLUX_MOBILITY,3,16,8},
{Op::FLOW_ORIGIN_RESERVE,3,16,8},
{Op::FLOW_PAIR_APPLY,3,16,8},
{Op::FRICTION_REST_DEPOSIT,3,16,8},
{Op::ORDERED_CAPTURE,3,16,8},
{Op::MICRO_AUDIT_SEAL,3,16,8},
{Op::ACTIVATE_REQUEST,3,16,9},
{Op::ACTIVATE_APPLY,3,16,9},
{Op::FLIGHT_PATH,3,16,9},
{Op::PARTICLE_FOLD,3,16,9},
{Op::FACE_FLUX_MOBILITY,3,16,9},
{Op::FLOW_ORIGIN_RESERVE,3,16,9},
{Op::FLOW_PAIR_APPLY,3,16,9},
{Op::FRICTION_REST_DEPOSIT,3,16,9},
{Op::ORDERED_CAPTURE,3,16,9},
{Op::MICRO_AUDIT_SEAL,3,16,9},
{Op::ACTIVATE_REQUEST,3,16,10},
{Op::ACTIVATE_APPLY,3,16,10},
{Op::FLIGHT_PATH,3,16,10},
{Op::PARTICLE_FOLD,3,16,10},
{Op::FACE_FLUX_MOBILITY,3,16,10},
{Op::FLOW_ORIGIN_RESERVE,3,16,10},
{Op::FLOW_PAIR_APPLY,3,16,10},
{Op::FRICTION_REST_DEPOSIT,3,16,10},
{Op::ORDERED_CAPTURE,3,16,10},
{Op::MICRO_AUDIT_SEAL,3,16,10},
{Op::ACTIVATE_REQUEST,3,16,11},
{Op::ACTIVATE_APPLY,3,16,11},
{Op::FLIGHT_PATH,3,16,11},
{Op::PARTICLE_FOLD,3,16,11},
{Op::FACE_FLUX_MOBILITY,3,16,11},
{Op::FLOW_ORIGIN_RESERVE,3,16,11},
{Op::FLOW_PAIR_APPLY,3,16,11},
{Op::FRICTION_REST_DEPOSIT,3,16,11},
{Op::ORDERED_CAPTURE,3,16,11},
{Op::MICRO_AUDIT_SEAL,3,16,11},
{Op::ACTIVATE_REQUEST,3,16,12},
{Op::ACTIVATE_APPLY,3,16,12},
{Op::FLIGHT_PATH,3,16,12},
{Op::PARTICLE_FOLD,3,16,12},
{Op::FACE_FLUX_MOBILITY,3,16,12},
{Op::FLOW_ORIGIN_RESERVE,3,16,12},
{Op::FLOW_PAIR_APPLY,3,16,12},
{Op::FRICTION_REST_DEPOSIT,3,16,12},
{Op::ORDERED_CAPTURE,3,16,12},
{Op::MICRO_AUDIT_SEAL,3,16,12},
{Op::ACTIVATE_REQUEST,3,16,13},
{Op::ACTIVATE_APPLY,3,16,13},
{Op::FLIGHT_PATH,3,16,13},
{Op::PARTICLE_FOLD,3,16,13},
{Op::FACE_FLUX_MOBILITY,3,16,13},
{Op::FLOW_ORIGIN_RESERVE,3,16,13},
{Op::FLOW_PAIR_APPLY,3,16,13},
{Op::FRICTION_REST_DEPOSIT,3,16,13},
{Op::ORDERED_CAPTURE,3,16,13},
{Op::MICRO_AUDIT_SEAL,3,16,13},
{Op::ACTIVATE_REQUEST,3,16,14},
{Op::ACTIVATE_APPLY,3,16,14},
{Op::FLIGHT_PATH,3,16,14},
{Op::PARTICLE_FOLD,3,16,14},
{Op::FACE_FLUX_MOBILITY,3,16,14},
{Op::FLOW_ORIGIN_RESERVE,3,16,14},
{Op::FLOW_PAIR_APPLY,3,16,14},
{Op::FRICTION_REST_DEPOSIT,3,16,14},
{Op::ORDERED_CAPTURE,3,16,14},
{Op::MICRO_AUDIT_SEAL,3,16,14},
{Op::ACTIVATE_REQUEST,3,16,15},
{Op::ACTIVATE_APPLY,3,16,15},
{Op::FLIGHT_PATH,3,16,15},
{Op::PARTICLE_FOLD,3,16,15},
{Op::FACE_FLUX_MOBILITY,3,16,15},
{Op::FLOW_ORIGIN_RESERVE,3,16,15},
{Op::FLOW_PAIR_APPLY,3,16,15},
{Op::FRICTION_REST_DEPOSIT,3,16,15},
{Op::ORDERED_CAPTURE,3,16,15},
{Op::MICRO_AUDIT_SEAL,3,16,15},
{Op::FINAL_MASS_ORIGIN_MOMENTUM,3,16,-1},
{Op::FINAL_ENERGY_CURSORS,3,16,-1},
{Op::SCOPE_SEAL_AND_CHOICE,3,16,-1},
{Op::INIT_COPY,3,32,-1},
{Op::INPUT_AND_CFL,3,32,-1},
{Op::GRAVITY_WORK,3,32,-1},
{Op::TOUCH_ATOMIC,3,32,-1},
{Op::AFTER_TOUCH_CFL,3,32,-1},
{Op::ACTIVATE_REQUEST,3,32,0},
{Op::ACTIVATE_APPLY,3,32,0},
{Op::FLIGHT_PATH,3,32,0},
{Op::PARTICLE_FOLD,3,32,0},
{Op::FACE_FLUX_MOBILITY,3,32,0},
{Op::FLOW_ORIGIN_RESERVE,3,32,0},
{Op::FLOW_PAIR_APPLY,3,32,0},
{Op::FRICTION_REST_DEPOSIT,3,32,0},
{Op::ORDERED_CAPTURE,3,32,0},
{Op::MICRO_AUDIT_SEAL,3,32,0},
{Op::ACTIVATE_REQUEST,3,32,1},
{Op::ACTIVATE_APPLY,3,32,1},
{Op::FLIGHT_PATH,3,32,1},
{Op::PARTICLE_FOLD,3,32,1},
{Op::FACE_FLUX_MOBILITY,3,32,1},
{Op::FLOW_ORIGIN_RESERVE,3,32,1},
{Op::FLOW_PAIR_APPLY,3,32,1},
{Op::FRICTION_REST_DEPOSIT,3,32,1},
{Op::ORDERED_CAPTURE,3,32,1},
{Op::MICRO_AUDIT_SEAL,3,32,1},
{Op::ACTIVATE_REQUEST,3,32,2},
{Op::ACTIVATE_APPLY,3,32,2},
{Op::FLIGHT_PATH,3,32,2},
{Op::PARTICLE_FOLD,3,32,2},
{Op::FACE_FLUX_MOBILITY,3,32,2},
{Op::FLOW_ORIGIN_RESERVE,3,32,2},
{Op::FLOW_PAIR_APPLY,3,32,2},
{Op::FRICTION_REST_DEPOSIT,3,32,2},
{Op::ORDERED_CAPTURE,3,32,2},
{Op::MICRO_AUDIT_SEAL,3,32,2},
{Op::ACTIVATE_REQUEST,3,32,3},
{Op::ACTIVATE_APPLY,3,32,3},
{Op::FLIGHT_PATH,3,32,3},
{Op::PARTICLE_FOLD,3,32,3},
{Op::FACE_FLUX_MOBILITY,3,32,3},
{Op::FLOW_ORIGIN_RESERVE,3,32,3},
{Op::FLOW_PAIR_APPLY,3,32,3},
{Op::FRICTION_REST_DEPOSIT,3,32,3},
{Op::ORDERED_CAPTURE,3,32,3},
{Op::MICRO_AUDIT_SEAL,3,32,3},
{Op::ACTIVATE_REQUEST,3,32,4},
{Op::ACTIVATE_APPLY,3,32,4},
{Op::FLIGHT_PATH,3,32,4},
{Op::PARTICLE_FOLD,3,32,4},
{Op::FACE_FLUX_MOBILITY,3,32,4},
{Op::FLOW_ORIGIN_RESERVE,3,32,4},
{Op::FLOW_PAIR_APPLY,3,32,4},
{Op::FRICTION_REST_DEPOSIT,3,32,4},
{Op::ORDERED_CAPTURE,3,32,4},
{Op::MICRO_AUDIT_SEAL,3,32,4},
{Op::ACTIVATE_REQUEST,3,32,5},
{Op::ACTIVATE_APPLY,3,32,5},
{Op::FLIGHT_PATH,3,32,5},
{Op::PARTICLE_FOLD,3,32,5},
{Op::FACE_FLUX_MOBILITY,3,32,5},
{Op::FLOW_ORIGIN_RESERVE,3,32,5},
{Op::FLOW_PAIR_APPLY,3,32,5},
{Op::FRICTION_REST_DEPOSIT,3,32,5},
{Op::ORDERED_CAPTURE,3,32,5},
{Op::MICRO_AUDIT_SEAL,3,32,5},
{Op::ACTIVATE_REQUEST,3,32,6},
{Op::ACTIVATE_APPLY,3,32,6},
{Op::FLIGHT_PATH,3,32,6},
{Op::PARTICLE_FOLD,3,32,6},
{Op::FACE_FLUX_MOBILITY,3,32,6},
{Op::FLOW_ORIGIN_RESERVE,3,32,6},
{Op::FLOW_PAIR_APPLY,3,32,6},
{Op::FRICTION_REST_DEPOSIT,3,32,6},
{Op::ORDERED_CAPTURE,3,32,6},
{Op::MICRO_AUDIT_SEAL,3,32,6},
{Op::ACTIVATE_REQUEST,3,32,7},
{Op::ACTIVATE_APPLY,3,32,7},
{Op::FLIGHT_PATH,3,32,7},
{Op::PARTICLE_FOLD,3,32,7},
{Op::FACE_FLUX_MOBILITY,3,32,7},
{Op::FLOW_ORIGIN_RESERVE,3,32,7},
{Op::FLOW_PAIR_APPLY,3,32,7},
{Op::FRICTION_REST_DEPOSIT,3,32,7},
{Op::ORDERED_CAPTURE,3,32,7},
{Op::MICRO_AUDIT_SEAL,3,32,7},
{Op::ACTIVATE_REQUEST,3,32,8},
{Op::ACTIVATE_APPLY,3,32,8},
{Op::FLIGHT_PATH,3,32,8},
{Op::PARTICLE_FOLD,3,32,8},
{Op::FACE_FLUX_MOBILITY,3,32,8},
{Op::FLOW_ORIGIN_RESERVE,3,32,8},
{Op::FLOW_PAIR_APPLY,3,32,8},
{Op::FRICTION_REST_DEPOSIT,3,32,8},
{Op::ORDERED_CAPTURE,3,32,8},
{Op::MICRO_AUDIT_SEAL,3,32,8},
{Op::ACTIVATE_REQUEST,3,32,9},
{Op::ACTIVATE_APPLY,3,32,9},
{Op::FLIGHT_PATH,3,32,9},
{Op::PARTICLE_FOLD,3,32,9},
{Op::FACE_FLUX_MOBILITY,3,32,9},
{Op::FLOW_ORIGIN_RESERVE,3,32,9},
{Op::FLOW_PAIR_APPLY,3,32,9},
{Op::FRICTION_REST_DEPOSIT,3,32,9},
{Op::ORDERED_CAPTURE,3,32,9},
{Op::MICRO_AUDIT_SEAL,3,32,9},
{Op::ACTIVATE_REQUEST,3,32,10},
{Op::ACTIVATE_APPLY,3,32,10},
{Op::FLIGHT_PATH,3,32,10},
{Op::PARTICLE_FOLD,3,32,10},
{Op::FACE_FLUX_MOBILITY,3,32,10},
{Op::FLOW_ORIGIN_RESERVE,3,32,10},
{Op::FLOW_PAIR_APPLY,3,32,10},
{Op::FRICTION_REST_DEPOSIT,3,32,10},
{Op::ORDERED_CAPTURE,3,32,10},
{Op::MICRO_AUDIT_SEAL,3,32,10},
{Op::ACTIVATE_REQUEST,3,32,11},
{Op::ACTIVATE_APPLY,3,32,11},
{Op::FLIGHT_PATH,3,32,11},
{Op::PARTICLE_FOLD,3,32,11},
{Op::FACE_FLUX_MOBILITY,3,32,11},
{Op::FLOW_ORIGIN_RESERVE,3,32,11},
{Op::FLOW_PAIR_APPLY,3,32,11},
{Op::FRICTION_REST_DEPOSIT,3,32,11},
{Op::ORDERED_CAPTURE,3,32,11},
{Op::MICRO_AUDIT_SEAL,3,32,11},
{Op::ACTIVATE_REQUEST,3,32,12},
{Op::ACTIVATE_APPLY,3,32,12},
{Op::FLIGHT_PATH,3,32,12},
{Op::PARTICLE_FOLD,3,32,12},
{Op::FACE_FLUX_MOBILITY,3,32,12},
{Op::FLOW_ORIGIN_RESERVE,3,32,12},
{Op::FLOW_PAIR_APPLY,3,32,12},
{Op::FRICTION_REST_DEPOSIT,3,32,12},
{Op::ORDERED_CAPTURE,3,32,12},
{Op::MICRO_AUDIT_SEAL,3,32,12},
{Op::ACTIVATE_REQUEST,3,32,13},
{Op::ACTIVATE_APPLY,3,32,13},
{Op::FLIGHT_PATH,3,32,13},
{Op::PARTICLE_FOLD,3,32,13},
{Op::FACE_FLUX_MOBILITY,3,32,13},
{Op::FLOW_ORIGIN_RESERVE,3,32,13},
{Op::FLOW_PAIR_APPLY,3,32,13},
{Op::FRICTION_REST_DEPOSIT,3,32,13},
{Op::ORDERED_CAPTURE,3,32,13},
{Op::MICRO_AUDIT_SEAL,3,32,13},
{Op::ACTIVATE_REQUEST,3,32,14},
{Op::ACTIVATE_APPLY,3,32,14},
{Op::FLIGHT_PATH,3,32,14},
{Op::PARTICLE_FOLD,3,32,14},
{Op::FACE_FLUX_MOBILITY,3,32,14},
{Op::FLOW_ORIGIN_RESERVE,3,32,14},
{Op::FLOW_PAIR_APPLY,3,32,14},
{Op::FRICTION_REST_DEPOSIT,3,32,14},
{Op::ORDERED_CAPTURE,3,32,14},
{Op::MICRO_AUDIT_SEAL,3,32,14},
{Op::ACTIVATE_REQUEST,3,32,15},
{Op::ACTIVATE_APPLY,3,32,15},
{Op::FLIGHT_PATH,3,32,15},
{Op::PARTICLE_FOLD,3,32,15},
{Op::FACE_FLUX_MOBILITY,3,32,15},
{Op::FLOW_ORIGIN_RESERVE,3,32,15},
{Op::FLOW_PAIR_APPLY,3,32,15},
{Op::FRICTION_REST_DEPOSIT,3,32,15},
{Op::ORDERED_CAPTURE,3,32,15},
{Op::MICRO_AUDIT_SEAL,3,32,15},
{Op::ACTIVATE_REQUEST,3,32,16},
{Op::ACTIVATE_APPLY,3,32,16},
{Op::FLIGHT_PATH,3,32,16},
{Op::PARTICLE_FOLD,3,32,16},
{Op::FACE_FLUX_MOBILITY,3,32,16},
{Op::FLOW_ORIGIN_RESERVE,3,32,16},
{Op::FLOW_PAIR_APPLY,3,32,16},
{Op::FRICTION_REST_DEPOSIT,3,32,16},
{Op::ORDERED_CAPTURE,3,32,16},
{Op::MICRO_AUDIT_SEAL,3,32,16},
{Op::ACTIVATE_REQUEST,3,32,17},
{Op::ACTIVATE_APPLY,3,32,17},
{Op::FLIGHT_PATH,3,32,17},
{Op::PARTICLE_FOLD,3,32,17},
{Op::FACE_FLUX_MOBILITY,3,32,17},
{Op::FLOW_ORIGIN_RESERVE,3,32,17},
{Op::FLOW_PAIR_APPLY,3,32,17},
{Op::FRICTION_REST_DEPOSIT,3,32,17},
{Op::ORDERED_CAPTURE,3,32,17},
{Op::MICRO_AUDIT_SEAL,3,32,17},
{Op::ACTIVATE_REQUEST,3,32,18},
{Op::ACTIVATE_APPLY,3,32,18},
{Op::FLIGHT_PATH,3,32,18},
{Op::PARTICLE_FOLD,3,32,18},
{Op::FACE_FLUX_MOBILITY,3,32,18},
{Op::FLOW_ORIGIN_RESERVE,3,32,18},
{Op::FLOW_PAIR_APPLY,3,32,18},
{Op::FRICTION_REST_DEPOSIT,3,32,18},
{Op::ORDERED_CAPTURE,3,32,18},
{Op::MICRO_AUDIT_SEAL,3,32,18},
{Op::ACTIVATE_REQUEST,3,32,19},
{Op::ACTIVATE_APPLY,3,32,19},
{Op::FLIGHT_PATH,3,32,19},
{Op::PARTICLE_FOLD,3,32,19},
{Op::FACE_FLUX_MOBILITY,3,32,19},
{Op::FLOW_ORIGIN_RESERVE,3,32,19},
{Op::FLOW_PAIR_APPLY,3,32,19},
{Op::FRICTION_REST_DEPOSIT,3,32,19},
{Op::ORDERED_CAPTURE,3,32,19},
{Op::MICRO_AUDIT_SEAL,3,32,19},
{Op::ACTIVATE_REQUEST,3,32,20},
{Op::ACTIVATE_APPLY,3,32,20},
{Op::FLIGHT_PATH,3,32,20},
{Op::PARTICLE_FOLD,3,32,20},
{Op::FACE_FLUX_MOBILITY,3,32,20},
{Op::FLOW_ORIGIN_RESERVE,3,32,20},
{Op::FLOW_PAIR_APPLY,3,32,20},
{Op::FRICTION_REST_DEPOSIT,3,32,20},
{Op::ORDERED_CAPTURE,3,32,20},
{Op::MICRO_AUDIT_SEAL,3,32,20},
{Op::ACTIVATE_REQUEST,3,32,21},
{Op::ACTIVATE_APPLY,3,32,21},
{Op::FLIGHT_PATH,3,32,21},
{Op::PARTICLE_FOLD,3,32,21},
{Op::FACE_FLUX_MOBILITY,3,32,21},
{Op::FLOW_ORIGIN_RESERVE,3,32,21},
{Op::FLOW_PAIR_APPLY,3,32,21},
{Op::FRICTION_REST_DEPOSIT,3,32,21},
{Op::ORDERED_CAPTURE,3,32,21},
{Op::MICRO_AUDIT_SEAL,3,32,21},
{Op::ACTIVATE_REQUEST,3,32,22},
{Op::ACTIVATE_APPLY,3,32,22},
{Op::FLIGHT_PATH,3,32,22},
{Op::PARTICLE_FOLD,3,32,22},
{Op::FACE_FLUX_MOBILITY,3,32,22},
{Op::FLOW_ORIGIN_RESERVE,3,32,22},
{Op::FLOW_PAIR_APPLY,3,32,22},
{Op::FRICTION_REST_DEPOSIT,3,32,22},
{Op::ORDERED_CAPTURE,3,32,22},
{Op::MICRO_AUDIT_SEAL,3,32,22},
{Op::ACTIVATE_REQUEST,3,32,23},
{Op::ACTIVATE_APPLY,3,32,23},
{Op::FLIGHT_PATH,3,32,23},
{Op::PARTICLE_FOLD,3,32,23},
{Op::FACE_FLUX_MOBILITY,3,32,23},
{Op::FLOW_ORIGIN_RESERVE,3,32,23},
{Op::FLOW_PAIR_APPLY,3,32,23},
{Op::FRICTION_REST_DEPOSIT,3,32,23},
{Op::ORDERED_CAPTURE,3,32,23},
{Op::MICRO_AUDIT_SEAL,3,32,23},
{Op::ACTIVATE_REQUEST,3,32,24},
{Op::ACTIVATE_APPLY,3,32,24},
{Op::FLIGHT_PATH,3,32,24},
{Op::PARTICLE_FOLD,3,32,24},
{Op::FACE_FLUX_MOBILITY,3,32,24},
{Op::FLOW_ORIGIN_RESERVE,3,32,24},
{Op::FLOW_PAIR_APPLY,3,32,24},
{Op::FRICTION_REST_DEPOSIT,3,32,24},
{Op::ORDERED_CAPTURE,3,32,24},
{Op::MICRO_AUDIT_SEAL,3,32,24},
{Op::ACTIVATE_REQUEST,3,32,25},
{Op::ACTIVATE_APPLY,3,32,25},
{Op::FLIGHT_PATH,3,32,25},
{Op::PARTICLE_FOLD,3,32,25},
{Op::FACE_FLUX_MOBILITY,3,32,25},
{Op::FLOW_ORIGIN_RESERVE,3,32,25},
{Op::FLOW_PAIR_APPLY,3,32,25},
{Op::FRICTION_REST_DEPOSIT,3,32,25},
{Op::ORDERED_CAPTURE,3,32,25},
{Op::MICRO_AUDIT_SEAL,3,32,25},
{Op::ACTIVATE_REQUEST,3,32,26},
{Op::ACTIVATE_APPLY,3,32,26},
{Op::FLIGHT_PATH,3,32,26},
{Op::PARTICLE_FOLD,3,32,26},
{Op::FACE_FLUX_MOBILITY,3,32,26},
{Op::FLOW_ORIGIN_RESERVE,3,32,26},
{Op::FLOW_PAIR_APPLY,3,32,26},
{Op::FRICTION_REST_DEPOSIT,3,32,26},
{Op::ORDERED_CAPTURE,3,32,26},
{Op::MICRO_AUDIT_SEAL,3,32,26},
{Op::ACTIVATE_REQUEST,3,32,27},
{Op::ACTIVATE_APPLY,3,32,27},
{Op::FLIGHT_PATH,3,32,27},
{Op::PARTICLE_FOLD,3,32,27},
{Op::FACE_FLUX_MOBILITY,3,32,27},
{Op::FLOW_ORIGIN_RESERVE,3,32,27},
{Op::FLOW_PAIR_APPLY,3,32,27},
{Op::FRICTION_REST_DEPOSIT,3,32,27},
{Op::ORDERED_CAPTURE,3,32,27},
{Op::MICRO_AUDIT_SEAL,3,32,27},
{Op::ACTIVATE_REQUEST,3,32,28},
{Op::ACTIVATE_APPLY,3,32,28},
{Op::FLIGHT_PATH,3,32,28},
{Op::PARTICLE_FOLD,3,32,28},
{Op::FACE_FLUX_MOBILITY,3,32,28},
{Op::FLOW_ORIGIN_RESERVE,3,32,28},
{Op::FLOW_PAIR_APPLY,3,32,28},
{Op::FRICTION_REST_DEPOSIT,3,32,28},
{Op::ORDERED_CAPTURE,3,32,28},
{Op::MICRO_AUDIT_SEAL,3,32,28},
{Op::ACTIVATE_REQUEST,3,32,29},
{Op::ACTIVATE_APPLY,3,32,29},
{Op::FLIGHT_PATH,3,32,29},
{Op::PARTICLE_FOLD,3,32,29},
{Op::FACE_FLUX_MOBILITY,3,32,29},
{Op::FLOW_ORIGIN_RESERVE,3,32,29},
{Op::FLOW_PAIR_APPLY,3,32,29},
{Op::FRICTION_REST_DEPOSIT,3,32,29},
{Op::ORDERED_CAPTURE,3,32,29},
{Op::MICRO_AUDIT_SEAL,3,32,29},
{Op::ACTIVATE_REQUEST,3,32,30},
{Op::ACTIVATE_APPLY,3,32,30},
{Op::FLIGHT_PATH,3,32,30},
{Op::PARTICLE_FOLD,3,32,30},
{Op::FACE_FLUX_MOBILITY,3,32,30},
{Op::FLOW_ORIGIN_RESERVE,3,32,30},
{Op::FLOW_PAIR_APPLY,3,32,30},
{Op::FRICTION_REST_DEPOSIT,3,32,30},
{Op::ORDERED_CAPTURE,3,32,30},
{Op::MICRO_AUDIT_SEAL,3,32,30},
{Op::ACTIVATE_REQUEST,3,32,31},
{Op::ACTIVATE_APPLY,3,32,31},
{Op::FLIGHT_PATH,3,32,31},
{Op::PARTICLE_FOLD,3,32,31},
{Op::FACE_FLUX_MOBILITY,3,32,31},
{Op::FLOW_ORIGIN_RESERVE,3,32,31},
{Op::FLOW_PAIR_APPLY,3,32,31},
{Op::FRICTION_REST_DEPOSIT,3,32,31},
{Op::ORDERED_CAPTURE,3,32,31},
{Op::MICRO_AUDIT_SEAL,3,32,31},
{Op::FINAL_MASS_ORIGIN_MOMENTUM,3,32,-1},
{Op::FINAL_ENERGY_CURSORS,3,32,-1},
{Op::SCOPE_SEAL_AND_CHOICE,3,32,-1},
{Op::REPLAY_BEGIN,0,0,-1},
{Op::INIT_COPY,4,32,-1},
{Op::INPUT_AND_CFL,4,32,-1},
{Op::GRAVITY_WORK,4,32,-1},
{Op::TOUCH_ATOMIC,4,32,-1},
{Op::AFTER_TOUCH_CFL,4,32,-1},
{Op::ACTIVATE_REQUEST,4,32,0},
{Op::ACTIVATE_APPLY,4,32,0},
{Op::FLIGHT_PATH,4,32,0},
{Op::PARTICLE_FOLD,4,32,0},
{Op::FACE_FLUX_MOBILITY,4,32,0},
{Op::FLOW_ORIGIN_RESERVE,4,32,0},
{Op::FLOW_PAIR_APPLY,4,32,0},
{Op::FRICTION_REST_DEPOSIT,4,32,0},
{Op::ORDERED_CAPTURE,4,32,0},
{Op::MICRO_AUDIT_SEAL,4,32,0},
{Op::ACTIVATE_REQUEST,4,32,1},
{Op::ACTIVATE_APPLY,4,32,1},
{Op::FLIGHT_PATH,4,32,1},
{Op::PARTICLE_FOLD,4,32,1},
{Op::FACE_FLUX_MOBILITY,4,32,1},
{Op::FLOW_ORIGIN_RESERVE,4,32,1},
{Op::FLOW_PAIR_APPLY,4,32,1},
{Op::FRICTION_REST_DEPOSIT,4,32,1},
{Op::ORDERED_CAPTURE,4,32,1},
{Op::MICRO_AUDIT_SEAL,4,32,1},
{Op::ACTIVATE_REQUEST,4,32,2},
{Op::ACTIVATE_APPLY,4,32,2},
{Op::FLIGHT_PATH,4,32,2},
{Op::PARTICLE_FOLD,4,32,2},
{Op::FACE_FLUX_MOBILITY,4,32,2},
{Op::FLOW_ORIGIN_RESERVE,4,32,2},
{Op::FLOW_PAIR_APPLY,4,32,2},
{Op::FRICTION_REST_DEPOSIT,4,32,2},
{Op::ORDERED_CAPTURE,4,32,2},
{Op::MICRO_AUDIT_SEAL,4,32,2},
{Op::ACTIVATE_REQUEST,4,32,3},
{Op::ACTIVATE_APPLY,4,32,3},
{Op::FLIGHT_PATH,4,32,3},
{Op::PARTICLE_FOLD,4,32,3},
{Op::FACE_FLUX_MOBILITY,4,32,3},
{Op::FLOW_ORIGIN_RESERVE,4,32,3},
{Op::FLOW_PAIR_APPLY,4,32,3},
{Op::FRICTION_REST_DEPOSIT,4,32,3},
{Op::ORDERED_CAPTURE,4,32,3},
{Op::MICRO_AUDIT_SEAL,4,32,3},
{Op::ACTIVATE_REQUEST,4,32,4},
{Op::ACTIVATE_APPLY,4,32,4},
{Op::FLIGHT_PATH,4,32,4},
{Op::PARTICLE_FOLD,4,32,4},
{Op::FACE_FLUX_MOBILITY,4,32,4},
{Op::FLOW_ORIGIN_RESERVE,4,32,4},
{Op::FLOW_PAIR_APPLY,4,32,4},
{Op::FRICTION_REST_DEPOSIT,4,32,4},
{Op::ORDERED_CAPTURE,4,32,4},
{Op::MICRO_AUDIT_SEAL,4,32,4},
{Op::ACTIVATE_REQUEST,4,32,5},
{Op::ACTIVATE_APPLY,4,32,5},
{Op::FLIGHT_PATH,4,32,5},
{Op::PARTICLE_FOLD,4,32,5},
{Op::FACE_FLUX_MOBILITY,4,32,5},
{Op::FLOW_ORIGIN_RESERVE,4,32,5},
{Op::FLOW_PAIR_APPLY,4,32,5},
{Op::FRICTION_REST_DEPOSIT,4,32,5},
{Op::ORDERED_CAPTURE,4,32,5},
{Op::MICRO_AUDIT_SEAL,4,32,5},
{Op::ACTIVATE_REQUEST,4,32,6},
{Op::ACTIVATE_APPLY,4,32,6},
{Op::FLIGHT_PATH,4,32,6},
{Op::PARTICLE_FOLD,4,32,6},
{Op::FACE_FLUX_MOBILITY,4,32,6},
{Op::FLOW_ORIGIN_RESERVE,4,32,6},
{Op::FLOW_PAIR_APPLY,4,32,6},
{Op::FRICTION_REST_DEPOSIT,4,32,6},
{Op::ORDERED_CAPTURE,4,32,6},
{Op::MICRO_AUDIT_SEAL,4,32,6},
{Op::ACTIVATE_REQUEST,4,32,7},
{Op::ACTIVATE_APPLY,4,32,7},
{Op::FLIGHT_PATH,4,32,7},
{Op::PARTICLE_FOLD,4,32,7},
{Op::FACE_FLUX_MOBILITY,4,32,7},
{Op::FLOW_ORIGIN_RESERVE,4,32,7},
{Op::FLOW_PAIR_APPLY,4,32,7},
{Op::FRICTION_REST_DEPOSIT,4,32,7},
{Op::ORDERED_CAPTURE,4,32,7},
{Op::MICRO_AUDIT_SEAL,4,32,7},
{Op::ACTIVATE_REQUEST,4,32,8},
{Op::ACTIVATE_APPLY,4,32,8},
{Op::FLIGHT_PATH,4,32,8},
{Op::PARTICLE_FOLD,4,32,8},
{Op::FACE_FLUX_MOBILITY,4,32,8},
{Op::FLOW_ORIGIN_RESERVE,4,32,8},
{Op::FLOW_PAIR_APPLY,4,32,8},
{Op::FRICTION_REST_DEPOSIT,4,32,8},
{Op::ORDERED_CAPTURE,4,32,8},
{Op::MICRO_AUDIT_SEAL,4,32,8},
{Op::ACTIVATE_REQUEST,4,32,9},
{Op::ACTIVATE_APPLY,4,32,9},
{Op::FLIGHT_PATH,4,32,9},
{Op::PARTICLE_FOLD,4,32,9},
{Op::FACE_FLUX_MOBILITY,4,32,9},
{Op::FLOW_ORIGIN_RESERVE,4,32,9},
{Op::FLOW_PAIR_APPLY,4,32,9},
{Op::FRICTION_REST_DEPOSIT,4,32,9},
{Op::ORDERED_CAPTURE,4,32,9},
{Op::MICRO_AUDIT_SEAL,4,32,9},
{Op::ACTIVATE_REQUEST,4,32,10},
{Op::ACTIVATE_APPLY,4,32,10},
{Op::FLIGHT_PATH,4,32,10},
{Op::PARTICLE_FOLD,4,32,10},
{Op::FACE_FLUX_MOBILITY,4,32,10},
{Op::FLOW_ORIGIN_RESERVE,4,32,10},
{Op::FLOW_PAIR_APPLY,4,32,10},
{Op::FRICTION_REST_DEPOSIT,4,32,10},
{Op::ORDERED_CAPTURE,4,32,10},
{Op::MICRO_AUDIT_SEAL,4,32,10},
{Op::ACTIVATE_REQUEST,4,32,11},
{Op::ACTIVATE_APPLY,4,32,11},
{Op::FLIGHT_PATH,4,32,11},
{Op::PARTICLE_FOLD,4,32,11},
{Op::FACE_FLUX_MOBILITY,4,32,11},
{Op::FLOW_ORIGIN_RESERVE,4,32,11},
{Op::FLOW_PAIR_APPLY,4,32,11},
{Op::FRICTION_REST_DEPOSIT,4,32,11},
{Op::ORDERED_CAPTURE,4,32,11},
{Op::MICRO_AUDIT_SEAL,4,32,11},
{Op::ACTIVATE_REQUEST,4,32,12},
{Op::ACTIVATE_APPLY,4,32,12},
{Op::FLIGHT_PATH,4,32,12},
{Op::PARTICLE_FOLD,4,32,12},
{Op::FACE_FLUX_MOBILITY,4,32,12},
{Op::FLOW_ORIGIN_RESERVE,4,32,12},
{Op::FLOW_PAIR_APPLY,4,32,12},
{Op::FRICTION_REST_DEPOSIT,4,32,12},
{Op::ORDERED_CAPTURE,4,32,12},
{Op::MICRO_AUDIT_SEAL,4,32,12},
{Op::ACTIVATE_REQUEST,4,32,13},
{Op::ACTIVATE_APPLY,4,32,13},
{Op::FLIGHT_PATH,4,32,13},
{Op::PARTICLE_FOLD,4,32,13},
{Op::FACE_FLUX_MOBILITY,4,32,13},
{Op::FLOW_ORIGIN_RESERVE,4,32,13},
{Op::FLOW_PAIR_APPLY,4,32,13},
{Op::FRICTION_REST_DEPOSIT,4,32,13},
{Op::ORDERED_CAPTURE,4,32,13},
{Op::MICRO_AUDIT_SEAL,4,32,13},
{Op::ACTIVATE_REQUEST,4,32,14},
{Op::ACTIVATE_APPLY,4,32,14},
{Op::FLIGHT_PATH,4,32,14},
{Op::PARTICLE_FOLD,4,32,14},
{Op::FACE_FLUX_MOBILITY,4,32,14},
{Op::FLOW_ORIGIN_RESERVE,4,32,14},
{Op::FLOW_PAIR_APPLY,4,32,14},
{Op::FRICTION_REST_DEPOSIT,4,32,14},
{Op::ORDERED_CAPTURE,4,32,14},
{Op::MICRO_AUDIT_SEAL,4,32,14},
{Op::ACTIVATE_REQUEST,4,32,15},
{Op::ACTIVATE_APPLY,4,32,15},
{Op::FLIGHT_PATH,4,32,15},
{Op::PARTICLE_FOLD,4,32,15},
{Op::FACE_FLUX_MOBILITY,4,32,15},
{Op::FLOW_ORIGIN_RESERVE,4,32,15},
{Op::FLOW_PAIR_APPLY,4,32,15},
{Op::FRICTION_REST_DEPOSIT,4,32,15},
{Op::ORDERED_CAPTURE,4,32,15},
{Op::MICRO_AUDIT_SEAL,4,32,15},
{Op::ACTIVATE_REQUEST,4,32,16},
{Op::ACTIVATE_APPLY,4,32,16},
{Op::FLIGHT_PATH,4,32,16},
{Op::PARTICLE_FOLD,4,32,16},
{Op::FACE_FLUX_MOBILITY,4,32,16},
{Op::FLOW_ORIGIN_RESERVE,4,32,16},
{Op::FLOW_PAIR_APPLY,4,32,16},
{Op::FRICTION_REST_DEPOSIT,4,32,16},
{Op::ORDERED_CAPTURE,4,32,16},
{Op::MICRO_AUDIT_SEAL,4,32,16},
{Op::ACTIVATE_REQUEST,4,32,17},
{Op::ACTIVATE_APPLY,4,32,17},
{Op::FLIGHT_PATH,4,32,17},
{Op::PARTICLE_FOLD,4,32,17},
{Op::FACE_FLUX_MOBILITY,4,32,17},
{Op::FLOW_ORIGIN_RESERVE,4,32,17},
{Op::FLOW_PAIR_APPLY,4,32,17},
{Op::FRICTION_REST_DEPOSIT,4,32,17},
{Op::ORDERED_CAPTURE,4,32,17},
{Op::MICRO_AUDIT_SEAL,4,32,17},
{Op::ACTIVATE_REQUEST,4,32,18},
{Op::ACTIVATE_APPLY,4,32,18},
{Op::FLIGHT_PATH,4,32,18},
{Op::PARTICLE_FOLD,4,32,18},
{Op::FACE_FLUX_MOBILITY,4,32,18},
{Op::FLOW_ORIGIN_RESERVE,4,32,18},
{Op::FLOW_PAIR_APPLY,4,32,18},
{Op::FRICTION_REST_DEPOSIT,4,32,18},
{Op::ORDERED_CAPTURE,4,32,18},
{Op::MICRO_AUDIT_SEAL,4,32,18},
{Op::ACTIVATE_REQUEST,4,32,19},
{Op::ACTIVATE_APPLY,4,32,19},
{Op::FLIGHT_PATH,4,32,19},
{Op::PARTICLE_FOLD,4,32,19},
{Op::FACE_FLUX_MOBILITY,4,32,19},
{Op::FLOW_ORIGIN_RESERVE,4,32,19},
{Op::FLOW_PAIR_APPLY,4,32,19},
{Op::FRICTION_REST_DEPOSIT,4,32,19},
{Op::ORDERED_CAPTURE,4,32,19},
{Op::MICRO_AUDIT_SEAL,4,32,19},
{Op::ACTIVATE_REQUEST,4,32,20},
{Op::ACTIVATE_APPLY,4,32,20},
{Op::FLIGHT_PATH,4,32,20},
{Op::PARTICLE_FOLD,4,32,20},
{Op::FACE_FLUX_MOBILITY,4,32,20},
{Op::FLOW_ORIGIN_RESERVE,4,32,20},
{Op::FLOW_PAIR_APPLY,4,32,20},
{Op::FRICTION_REST_DEPOSIT,4,32,20},
{Op::ORDERED_CAPTURE,4,32,20},
{Op::MICRO_AUDIT_SEAL,4,32,20},
{Op::ACTIVATE_REQUEST,4,32,21},
{Op::ACTIVATE_APPLY,4,32,21},
{Op::FLIGHT_PATH,4,32,21},
{Op::PARTICLE_FOLD,4,32,21},
{Op::FACE_FLUX_MOBILITY,4,32,21},
{Op::FLOW_ORIGIN_RESERVE,4,32,21},
{Op::FLOW_PAIR_APPLY,4,32,21},
{Op::FRICTION_REST_DEPOSIT,4,32,21},
{Op::ORDERED_CAPTURE,4,32,21},
{Op::MICRO_AUDIT_SEAL,4,32,21},
{Op::ACTIVATE_REQUEST,4,32,22},
{Op::ACTIVATE_APPLY,4,32,22},
{Op::FLIGHT_PATH,4,32,22},
{Op::PARTICLE_FOLD,4,32,22},
{Op::FACE_FLUX_MOBILITY,4,32,22},
{Op::FLOW_ORIGIN_RESERVE,4,32,22},
{Op::FLOW_PAIR_APPLY,4,32,22},
{Op::FRICTION_REST_DEPOSIT,4,32,22},
{Op::ORDERED_CAPTURE,4,32,22},
{Op::MICRO_AUDIT_SEAL,4,32,22},
{Op::ACTIVATE_REQUEST,4,32,23},
{Op::ACTIVATE_APPLY,4,32,23},
{Op::FLIGHT_PATH,4,32,23},
{Op::PARTICLE_FOLD,4,32,23},
{Op::FACE_FLUX_MOBILITY,4,32,23},
{Op::FLOW_ORIGIN_RESERVE,4,32,23},
{Op::FLOW_PAIR_APPLY,4,32,23},
{Op::FRICTION_REST_DEPOSIT,4,32,23},
{Op::ORDERED_CAPTURE,4,32,23},
{Op::MICRO_AUDIT_SEAL,4,32,23},
{Op::ACTIVATE_REQUEST,4,32,24},
{Op::ACTIVATE_APPLY,4,32,24},
{Op::FLIGHT_PATH,4,32,24},
{Op::PARTICLE_FOLD,4,32,24},
{Op::FACE_FLUX_MOBILITY,4,32,24},
{Op::FLOW_ORIGIN_RESERVE,4,32,24},
{Op::FLOW_PAIR_APPLY,4,32,24},
{Op::FRICTION_REST_DEPOSIT,4,32,24},
{Op::ORDERED_CAPTURE,4,32,24},
{Op::MICRO_AUDIT_SEAL,4,32,24},
{Op::ACTIVATE_REQUEST,4,32,25},
{Op::ACTIVATE_APPLY,4,32,25},
{Op::FLIGHT_PATH,4,32,25},
{Op::PARTICLE_FOLD,4,32,25},
{Op::FACE_FLUX_MOBILITY,4,32,25},
{Op::FLOW_ORIGIN_RESERVE,4,32,25},
{Op::FLOW_PAIR_APPLY,4,32,25},
{Op::FRICTION_REST_DEPOSIT,4,32,25},
{Op::ORDERED_CAPTURE,4,32,25},
{Op::MICRO_AUDIT_SEAL,4,32,25},
{Op::ACTIVATE_REQUEST,4,32,26},
{Op::ACTIVATE_APPLY,4,32,26},
{Op::FLIGHT_PATH,4,32,26},
{Op::PARTICLE_FOLD,4,32,26},
{Op::FACE_FLUX_MOBILITY,4,32,26},
{Op::FLOW_ORIGIN_RESERVE,4,32,26},
{Op::FLOW_PAIR_APPLY,4,32,26},
{Op::FRICTION_REST_DEPOSIT,4,32,26},
{Op::ORDERED_CAPTURE,4,32,26},
{Op::MICRO_AUDIT_SEAL,4,32,26},
{Op::ACTIVATE_REQUEST,4,32,27},
{Op::ACTIVATE_APPLY,4,32,27},
{Op::FLIGHT_PATH,4,32,27},
{Op::PARTICLE_FOLD,4,32,27},
{Op::FACE_FLUX_MOBILITY,4,32,27},
{Op::FLOW_ORIGIN_RESERVE,4,32,27},
{Op::FLOW_PAIR_APPLY,4,32,27},
{Op::FRICTION_REST_DEPOSIT,4,32,27},
{Op::ORDERED_CAPTURE,4,32,27},
{Op::MICRO_AUDIT_SEAL,4,32,27},
{Op::ACTIVATE_REQUEST,4,32,28},
{Op::ACTIVATE_APPLY,4,32,28},
{Op::FLIGHT_PATH,4,32,28},
{Op::PARTICLE_FOLD,4,32,28},
{Op::FACE_FLUX_MOBILITY,4,32,28},
{Op::FLOW_ORIGIN_RESERVE,4,32,28},
{Op::FLOW_PAIR_APPLY,4,32,28},
{Op::FRICTION_REST_DEPOSIT,4,32,28},
{Op::ORDERED_CAPTURE,4,32,28},
{Op::MICRO_AUDIT_SEAL,4,32,28},
{Op::ACTIVATE_REQUEST,4,32,29},
{Op::ACTIVATE_APPLY,4,32,29},
{Op::FLIGHT_PATH,4,32,29},
{Op::PARTICLE_FOLD,4,32,29},
{Op::FACE_FLUX_MOBILITY,4,32,29},
{Op::FLOW_ORIGIN_RESERVE,4,32,29},
{Op::FLOW_PAIR_APPLY,4,32,29},
{Op::FRICTION_REST_DEPOSIT,4,32,29},
{Op::ORDERED_CAPTURE,4,32,29},
{Op::MICRO_AUDIT_SEAL,4,32,29},
{Op::ACTIVATE_REQUEST,4,32,30},
{Op::ACTIVATE_APPLY,4,32,30},
{Op::FLIGHT_PATH,4,32,30},
{Op::PARTICLE_FOLD,4,32,30},
{Op::FACE_FLUX_MOBILITY,4,32,30},
{Op::FLOW_ORIGIN_RESERVE,4,32,30},
{Op::FLOW_PAIR_APPLY,4,32,30},
{Op::FRICTION_REST_DEPOSIT,4,32,30},
{Op::ORDERED_CAPTURE,4,32,30},
{Op::MICRO_AUDIT_SEAL,4,32,30},
{Op::ACTIVATE_REQUEST,4,32,31},
{Op::ACTIVATE_APPLY,4,32,31},
{Op::FLIGHT_PATH,4,32,31},
{Op::PARTICLE_FOLD,4,32,31},
{Op::FACE_FLUX_MOBILITY,4,32,31},
{Op::FLOW_ORIGIN_RESERVE,4,32,31},
{Op::FLOW_PAIR_APPLY,4,32,31},
{Op::FRICTION_REST_DEPOSIT,4,32,31},
{Op::ORDERED_CAPTURE,4,32,31},
{Op::MICRO_AUDIT_SEAL,4,32,31},
{Op::FINAL_MASS_ORIGIN_MOMENTUM,4,32,-1},
{Op::FINAL_ENERGY_CURSORS,4,32,-1},
{Op::SCOPE_SEAL_AND_CHOICE,4,32,-1},
{Op::COMPARE_SELECTED_AUTHORITATIVE,0,0,-1},
{Op::COMPARE_REPLAY_STREAM,0,0,-1},
{Op::PACK_SCENE_METADATA,0,0,-1},
{Op::ROOT_SEAL,0,0,-1},
}};
inline constexpr std::array<Pass,12> kSourcePasses={{
{Op::SOURCE_INIT_BASE,0,0,-1},
{Op::SOURCE_VALIDATE_INPUT,0,0,-1},
{Op::SOURCE_DEDUP,0,0,-1},
{Op::SOURCE_DECODE,0,0,-1},
{Op::SOURCE_CREDIT,0,0,-1},
{Op::SOURCE_PRIVATE_BIRTH,0,0,-1},
{Op::SOURCE_ORIGIN_CERTIFICATE,0,0,-1},
{Op::SOURCE_LEDGER,0,0,-1},
{Op::SOURCE_AUDIT_REPLAY,0,0,-1},
{Op::SOURCE_COMPARE,0,0,-1},
{Op::SOURCE_PACK_SCENE,0,0,-1},
{Op::SOURCE_SEAL,0,0,-1},
}};
struct Range{size_t begin,bytes;constexpr size_t End()const{return begin+bytes;}};
struct Profile{uint32_t k,eventCapacity,pathCapacity;size_t stateBytes,workBytes,arenaBytes,payloadBytes,rootColdBytes,fullColdBytes;std::array<Range,8> state;std::array<Range,15> work;};
inline constexpr std::array<Profile,2> kProfiles={{
{192,33488,49152,591872,1286784,11718656,51120768,24653824,94965760,{{{0,1024},{1024,512},{1536,512},{2048,16384},{18432,131072},{149504,131072},{280576,49152},{329728,262144}}},{{{0,16512},{16512,32768},{49280,1048576},{1097856,65536},{1163392,1536},{1164928,512},{1165440,1536},{1166976,4096},{1171072,1024},{1172096,1536},{1173632,512},{1174144,9216},{1183360,32768},{1216128,65536},{1281664,5120}}}},
{512,54288,131072,673792,1289344,22286336,93803648,45953024,179671040,{{{0,1024},{1024,512},{1536,512},{2048,16384},{18432,131072},{149504,131072},{280576,131072},{411648,262144}}},{{{0,16512},{16512,32768},{49280,1048576},{1097856,65536},{1163392,4096},{1167488,512},{1168000,1536},{1169536,4096},{1173632,1024},{1174656,1536},{1176192,512},{1176704,9216},{1185920,32768},{1218688,65536},{1284224,5120}}}},
}};
inline const Profile* SelectProfile(uint32_t k){for(const auto& p:kProfiles)if(p.k==k)return &p;return nullptr;}
}
