#pragma once
#include <array>
#include <cstdint>
namespace grs::g13 {
inline constexpr uint32_t kSeal=0xc0de1301u;
struct Pass { uint32_t op,scope,J,micro,block; };
inline constexpr uint32_t kMagic_profile=1177760071u;
inline constexpr uint32_t kMagic_config=1127428423u;
inline constexpr uint32_t kMagic_state=1395863879u;
inline constexpr uint32_t kMagic_input=1228091719u;
inline constexpr uint32_t kMagic_trace=1412641095u;
inline constexpr uint32_t kMagic_tail=1379086663u;
inline constexpr uint32_t kMagic_render=1446195527u;
inline constexpr uint32_t kMagic_cold=1261646151u;
inline constexpr uint32_t kMagic_view=1462972743u;
namespace Config128 {
inline constexpr uint32_t magic=0u;
inline constexpr uint32_t schema=1u;
inline constexpr uint32_t profile=2u;
inline constexpr uint32_t lengthWords=3u;
inline constexpr uint32_t Nx=4u;
inline constexpr uint32_t Ny=5u;
inline constexpr uint32_t boundary=6u;
inline constexpr uint32_t seedMode=7u;
inline constexpr uint32_t LxF32=8u;
inline constexpr uint32_t LyF32=9u;
inline constexpr uint32_t hF32=10u;
inline constexpr uint32_t T0F32=11u;
inline constexpr uint32_t L0Binary64_lo=12u;
inline constexpr uint32_t L0Binary64_hi=13u;
inline constexpr uint32_t rho0Binary64_lo=14u;
inline constexpr uint32_t rho0Binary64_hi=15u;
inline constexpr uint32_t muSIBinary64_lo=16u;
inline constexpr uint32_t muSIBinary64_hi=17u;
inline constexpr uint32_t nuF32=18u;
inline constexpr uint32_t unitPolicy=19u;
inline constexpr uint32_t motionMode=20u;
inline constexpr uint32_t OmF32=21u;
inline constexpr uint32_t durationF32=22u;
inline constexpr uint32_t theta0F32=23u;
inline constexpr uint32_t gxWorldF32=24u;
inline constexpr uint32_t gyWorldF32=25u;
inline constexpr uint32_t forceAmpF32=26u;
inline constexpr uint32_t forceKind=27u;
inline constexpr uint32_t tracerCount=28u;
inline constexpr uint32_t positiveWorkCapF32=29u;
inline constexpr uint32_t precisionPolicy=30u;
inline constexpr uint32_t gatesRevision=31u;
inline constexpr uint32_t wallBottomF32=32u;
inline constexpr uint32_t wallTopF32=33u;
inline constexpr uint32_t pressureReferenceF32=34u;
inline constexpr uint32_t reserved0_35=35u;
inline constexpr uint32_t algorithmSHA_0=36u;
inline constexpr uint32_t algorithmSHA_1=37u;
inline constexpr uint32_t algorithmSHA_2=38u;
inline constexpr uint32_t algorithmSHA_3=39u;
inline constexpr uint32_t algorithmSHA_4=40u;
inline constexpr uint32_t algorithmSHA_5=41u;
inline constexpr uint32_t algorithmSHA_6=42u;
inline constexpr uint32_t algorithmSHA_7=43u;
inline constexpr uint32_t layoutSHA_0=44u;
inline constexpr uint32_t layoutSHA_1=45u;
inline constexpr uint32_t layoutSHA_2=46u;
inline constexpr uint32_t layoutSHA_3=47u;
inline constexpr uint32_t layoutSHA_4=48u;
inline constexpr uint32_t layoutSHA_5=49u;
inline constexpr uint32_t layoutSHA_6=50u;
inline constexpr uint32_t layoutSHA_7=51u;
inline constexpr uint32_t gatesSHA_0=52u;
inline constexpr uint32_t gatesSHA_1=53u;
inline constexpr uint32_t gatesSHA_2=54u;
inline constexpr uint32_t gatesSHA_3=55u;
inline constexpr uint32_t gatesSHA_4=56u;
inline constexpr uint32_t gatesSHA_5=57u;
inline constexpr uint32_t gatesSHA_6=58u;
inline constexpr uint32_t gatesSHA_7=59u;
inline constexpr uint32_t seedSHA_0=60u;
inline constexpr uint32_t seedSHA_1=61u;
inline constexpr uint32_t seedSHA_2=62u;
inline constexpr uint32_t seedSHA_3=63u;
inline constexpr uint32_t seedSHA_4=64u;
inline constexpr uint32_t seedSHA_5=65u;
inline constexpr uint32_t seedSHA_6=66u;
inline constexpr uint32_t seedSHA_7=67u;
inline constexpr uint32_t fixtureSHA_0=68u;
inline constexpr uint32_t fixtureSHA_1=69u;
inline constexpr uint32_t fixtureSHA_2=70u;
inline constexpr uint32_t fixtureSHA_3=71u;
inline constexpr uint32_t fixtureSHA_4=72u;
inline constexpr uint32_t fixtureSHA_5=73u;
inline constexpr uint32_t fixtureSHA_6=74u;
inline constexpr uint32_t fixtureSHA_7=75u;
inline constexpr uint32_t configSHA_0=76u;
inline constexpr uint32_t configSHA_1=77u;
inline constexpr uint32_t configSHA_2=78u;
inline constexpr uint32_t configSHA_3=79u;
inline constexpr uint32_t configSHA_4=80u;
inline constexpr uint32_t configSHA_5=81u;
inline constexpr uint32_t configSHA_6=82u;
inline constexpr uint32_t configSHA_7=83u;
inline constexpr uint32_t reserved0_84=84u;
inline constexpr uint32_t reserved0_85=85u;
inline constexpr uint32_t reserved0_86=86u;
inline constexpr uint32_t reserved0_87=87u;
inline constexpr uint32_t reserved0_88=88u;
inline constexpr uint32_t reserved0_89=89u;
inline constexpr uint32_t reserved0_90=90u;
inline constexpr uint32_t reserved0_91=91u;
inline constexpr uint32_t reserved0_92=92u;
inline constexpr uint32_t reserved0_93=93u;
inline constexpr uint32_t reserved0_94=94u;
inline constexpr uint32_t reserved0_95=95u;
inline constexpr uint32_t reserved0_96=96u;
inline constexpr uint32_t reserved0_97=97u;
inline constexpr uint32_t reserved0_98=98u;
inline constexpr uint32_t reserved0_99=99u;
inline constexpr uint32_t reserved0_100=100u;
inline constexpr uint32_t reserved0_101=101u;
inline constexpr uint32_t reserved0_102=102u;
inline constexpr uint32_t reserved0_103=103u;
inline constexpr uint32_t reserved0_104=104u;
inline constexpr uint32_t reserved0_105=105u;
inline constexpr uint32_t reserved0_106=106u;
inline constexpr uint32_t reserved0_107=107u;
inline constexpr uint32_t reserved0_108=108u;
inline constexpr uint32_t reserved0_109=109u;
inline constexpr uint32_t reserved0_110=110u;
inline constexpr uint32_t reserved0_111=111u;
inline constexpr uint32_t reserved0_112=112u;
inline constexpr uint32_t reserved0_113=113u;
inline constexpr uint32_t reserved0_114=114u;
inline constexpr uint32_t reserved0_115=115u;
inline constexpr uint32_t reserved0_116=116u;
inline constexpr uint32_t reserved0_117=117u;
inline constexpr uint32_t reserved0_118=118u;
inline constexpr uint32_t reserved0_119=119u;
inline constexpr uint32_t reserved0_120=120u;
inline constexpr uint32_t reserved0_121=121u;
inline constexpr uint32_t reserved0_122=122u;
inline constexpr uint32_t reserved0_123=123u;
inline constexpr uint32_t reserved0_124=124u;
inline constexpr uint32_t reserved0_125=125u;
inline constexpr uint32_t reserved0_126=126u;
inline constexpr uint32_t reserved0_127=127u;
}
namespace StateHeader256 {
inline constexpr uint32_t magic=0u;
inline constexpr uint32_t schema=1u;
inline constexpr uint32_t profile=2u;
inline constexpr uint32_t lengthWords=3u;
inline constexpr uint32_t Nx=4u;
inline constexpr uint32_t Ny=5u;
inline constexpr uint32_t boundary=6u;
inline constexpr uint32_t status=7u;
inline constexpr uint32_t run_lo=8u;
inline constexpr uint32_t run_hi=9u;
inline constexpr uint32_t context_lo=10u;
inline constexpr uint32_t context_hi=11u;
inline constexpr uint32_t surface_lo=12u;
inline constexpr uint32_t surface_hi=13u;
inline constexpr uint32_t geometry_lo=14u;
inline constexpr uint32_t geometry_hi=15u;
inline constexpr uint32_t commit_lo=16u;
inline constexpr uint32_t commit_hi=17u;
inline constexpr uint32_t acceptedQuantum_lo=18u;
inline constexpr uint32_t acceptedQuantum_hi=19u;
inline constexpr uint32_t selectedJ=20u;
inline constexpr uint32_t rootKind=21u;
inline constexpr uint32_t sourceSeq_lo=22u;
inline constexpr uint32_t sourceSeq_hi=23u;
inline constexpr uint32_t motionOriginQuantum_lo=24u;
inline constexpr uint32_t motionOriginQuantum_hi=25u;
inline constexpr uint32_t theta0F32=26u;
inline constexpr uint32_t OmF32=27u;
inline constexpr uint32_t durationF32=28u;
inline constexpr uint32_t forceAmpF32_29=29u;
inline constexpr uint32_t motionMode_30=30u;
inline constexpr uint32_t motionFlags=31u;
inline constexpr uint32_t configSHA_0=32u;
inline constexpr uint32_t configSHA_1=33u;
inline constexpr uint32_t configSHA_2=34u;
inline constexpr uint32_t configSHA_3=35u;
inline constexpr uint32_t configSHA_4=36u;
inline constexpr uint32_t configSHA_5=37u;
inline constexpr uint32_t configSHA_6=38u;
inline constexpr uint32_t configSHA_7=39u;
inline constexpr uint32_t algorithmSHA_0=40u;
inline constexpr uint32_t algorithmSHA_1=41u;
inline constexpr uint32_t algorithmSHA_2=42u;
inline constexpr uint32_t algorithmSHA_3=43u;
inline constexpr uint32_t algorithmSHA_4=44u;
inline constexpr uint32_t algorithmSHA_5=45u;
inline constexpr uint32_t algorithmSHA_6=46u;
inline constexpr uint32_t algorithmSHA_7=47u;
inline constexpr uint32_t layoutSHA_0=48u;
inline constexpr uint32_t layoutSHA_1=49u;
inline constexpr uint32_t layoutSHA_2=50u;
inline constexpr uint32_t layoutSHA_3=51u;
inline constexpr uint32_t layoutSHA_4=52u;
inline constexpr uint32_t layoutSHA_5=53u;
inline constexpr uint32_t layoutSHA_6=54u;
inline constexpr uint32_t layoutSHA_7=55u;
inline constexpr uint32_t gatesSHA_0=56u;
inline constexpr uint32_t gatesSHA_1=57u;
inline constexpr uint32_t gatesSHA_2=58u;
inline constexpr uint32_t gatesSHA_3=59u;
inline constexpr uint32_t gatesSHA_4=60u;
inline constexpr uint32_t gatesSHA_5=61u;
inline constexpr uint32_t gatesSHA_6=62u;
inline constexpr uint32_t gatesSHA_7=63u;
inline constexpr uint32_t seedSHA_0=64u;
inline constexpr uint32_t seedSHA_1=65u;
inline constexpr uint32_t seedSHA_2=66u;
inline constexpr uint32_t seedSHA_3=67u;
inline constexpr uint32_t seedSHA_4=68u;
inline constexpr uint32_t seedSHA_5=69u;
inline constexpr uint32_t seedSHA_6=70u;
inline constexpr uint32_t seedSHA_7=71u;
inline constexpr uint32_t inputSHA_0=72u;
inline constexpr uint32_t inputSHA_1=73u;
inline constexpr uint32_t inputSHA_2=74u;
inline constexpr uint32_t inputSHA_3=75u;
inline constexpr uint32_t inputSHA_4=76u;
inline constexpr uint32_t inputSHA_5=77u;
inline constexpr uint32_t inputSHA_6=78u;
inline constexpr uint32_t inputSHA_7=79u;
inline constexpr uint32_t KwindowStart_valueF32=80u;
inline constexpr uint32_t KwindowStart_errorBoundF32=81u;
inline constexpr uint32_t K_valueF32=82u;
inline constexpr uint32_t K_errorBoundF32=83u;
inline constexpr uint32_t WEuler_valueF32=84u;
inline constexpr uint32_t WEuler_errorBoundF32=85u;
inline constexpr uint32_t Wnonpotential_valueF32=86u;
inline constexpr uint32_t Wnonpotential_errorBoundF32=87u;
inline constexpr uint32_t Wwall_valueF32=88u;
inline constexpr uint32_t Wwall_errorBoundF32=89u;
inline constexpr uint32_t Wpositive_valueF32=90u;
inline constexpr uint32_t Wpositive_errorBoundF32=91u;
inline constexpr uint32_t Dadv_valueF32=92u;
inline constexpr uint32_t Dadv_errorBoundF32=93u;
inline constexpr uint32_t Dnu_valueF32=94u;
inline constexpr uint32_t Dnu_errorBoundF32=95u;
inline constexpr uint32_t DBE_valueF32=96u;
inline constexpr uint32_t DBE_errorBoundF32=97u;
inline constexpr uint32_t Dprojection_valueF32=98u;
inline constexpr uint32_t Dprojection_errorBoundF32=99u;
inline constexpr uint32_t Rdualdiv_valueF32=100u;
inline constexpr uint32_t Rdualdiv_errorBoundF32=101u;
inline constexpr uint32_t RH_valueF32=102u;
inline constexpr uint32_t RH_errorBoundF32=103u;
inline constexpr uint32_t RP_valueF32=104u;
inline constexpr uint32_t RP_errorBoundF32=105u;
inline constexpr uint32_t RrhsQuantization_valueF32=106u;
inline constexpr uint32_t RrhsQuantization_errorBoundF32=107u;
inline constexpr uint32_t Rfloat_valueF32=108u;
inline constexpr uint32_t Rfloat_errorBoundF32=109u;
inline constexpr uint32_t windowAbsWork_valueF32=110u;
inline constexpr uint32_t windowAbsWork_errorBoundF32=111u;
inline constexpr uint32_t windowDissipation_valueF32=112u;
inline constexpr uint32_t windowDissipation_errorBoundF32=113u;
inline constexpr uint32_t Escalefrozen_valueF32=114u;
inline constexpr uint32_t Escalefrozen_errorBoundF32=115u;
inline constexpr uint32_t momentumClosureX_valueF32=116u;
inline constexpr uint32_t momentumClosureX_errorBoundF32=117u;
inline constexpr uint32_t momentumClosureY_valueF32=118u;
inline constexpr uint32_t momentumClosureY_errorBoundF32=119u;
inline constexpr uint32_t CFLincoming_valueF32=120u;
inline constexpr uint32_t CFLincoming_errorBoundF32=121u;
inline constexpr uint32_t CFLoutgoing_valueF32=122u;
inline constexpr uint32_t CFLoutgoing_errorBoundF32=123u;
inline constexpr uint32_t divergenceRMS_valueF32=124u;
inline constexpr uint32_t divergenceRMS_errorBoundF32=125u;
inline constexpr uint32_t divergenceMax_valueF32=126u;
inline constexpr uint32_t divergenceMax_errorBoundF32=127u;
inline constexpr uint32_t viscBlock=128u;
inline constexpr uint32_t viscIterations=129u;
inline constexpr uint32_t pressureCycle=130u;
inline constexpr uint32_t pressureSweeps=131u;
inline constexpr uint32_t rHRMSF32=132u;
inline constexpr uint32_t rHMaxF32=133u;
inline constexpr uint32_t rPMaxQ32_lo=134u;
inline constexpr uint32_t rPMaxQ32_hi=135u;
inline constexpr uint32_t rPSumSquares128_0=136u;
inline constexpr uint32_t rPSumSquares128_1=137u;
inline constexpr uint32_t rPSumSquares128_2=138u;
inline constexpr uint32_t rPSumSquares128_3=139u;
inline constexpr uint32_t piMeanSumQ32_lo=140u;
inline constexpr uint32_t piMeanSumQ32_hi=141u;
inline constexpr uint32_t lambdaLowerBoundF32=142u;
inline constexpr uint32_t etaBF32=143u;
inline constexpr uint32_t pressureCertificateF32=144u;
inline constexpr uint32_t velocityCertificateF32=145u;
inline constexpr uint32_t projectionIdentityMaxF32=146u;
inline constexpr uint32_t thetaWrappedF32=147u;
inline constexpr uint32_t OmegaF32=148u;
inline constexpr uint32_t alphaF32=149u;
inline constexpr uint32_t phiMeanF32=150u;
inline constexpr uint32_t pressureReferenceF32=151u;
inline constexpr uint32_t mathBackend=152u;
inline constexpr uint32_t firstFailedOp=153u;
inline constexpr uint32_t firstFailure=154u;
inline constexpr uint32_t activeMicro=155u;
inline constexpr uint32_t activeBlocks=156u;
inline constexpr uint32_t activeCycles=157u;
inline constexpr uint32_t noopDispatches=158u;
inline constexpr uint32_t qualityFlags=159u;
inline constexpr uint32_t wall0_pressureImpulseXF32=160u;
inline constexpr uint32_t wall0_pressureImpulseYF32=161u;
inline constexpr uint32_t wall0_viscousImpulseXF32=162u;
inline constexpr uint32_t wall0_viscousImpulseYF32=163u;
inline constexpr uint32_t wall0_workF32=164u;
inline constexpr uint32_t wall0_meanTotalPressureF32=165u;
inline constexpr uint32_t wall0_flags=166u;
inline constexpr uint32_t wall0_reserved0=167u;
inline constexpr uint32_t wall1_pressureImpulseXF32=168u;
inline constexpr uint32_t wall1_pressureImpulseYF32=169u;
inline constexpr uint32_t wall1_viscousImpulseXF32=170u;
inline constexpr uint32_t wall1_viscousImpulseYF32=171u;
inline constexpr uint32_t wall1_workF32=172u;
inline constexpr uint32_t wall1_meanTotalPressureF32=173u;
inline constexpr uint32_t wall1_flags=174u;
inline constexpr uint32_t wall1_reserved0=175u;
inline constexpr uint32_t wall2_pressureImpulseXF32=176u;
inline constexpr uint32_t wall2_pressureImpulseYF32=177u;
inline constexpr uint32_t wall2_viscousImpulseXF32=178u;
inline constexpr uint32_t wall2_viscousImpulseYF32=179u;
inline constexpr uint32_t wall2_workF32=180u;
inline constexpr uint32_t wall2_meanTotalPressureF32=181u;
inline constexpr uint32_t wall2_flags=182u;
inline constexpr uint32_t wall2_reserved0=183u;
inline constexpr uint32_t wall3_pressureImpulseXF32=184u;
inline constexpr uint32_t wall3_pressureImpulseYF32=185u;
inline constexpr uint32_t wall3_viscousImpulseXF32=186u;
inline constexpr uint32_t wall3_viscousImpulseYF32=187u;
inline constexpr uint32_t wall3_workF32=188u;
inline constexpr uint32_t wall3_meanTotalPressureF32=189u;
inline constexpr uint32_t wall3_flags=190u;
inline constexpr uint32_t wall3_reserved0=191u;
inline constexpr uint32_t consumedSourceSeq_lo=192u;
inline constexpr uint32_t consumedSourceSeq_hi=193u;
inline constexpr uint32_t sourceControlGeneration_lo=194u;
inline constexpr uint32_t sourceControlGeneration_hi=195u;
inline constexpr uint32_t sourceIntent_lo=196u;
inline constexpr uint32_t sourceIntent_hi=197u;
inline constexpr uint32_t forceStartQuantum_lo=198u;
inline constexpr uint32_t forceStartQuantum_hi=199u;
inline constexpr uint32_t forceEndQuantum_lo=200u;
inline constexpr uint32_t forceEndQuantum_hi=201u;
inline constexpr uint32_t gxWorldF32=202u;
inline constexpr uint32_t gyWorldF32=203u;
inline constexpr uint32_t forceKind=204u;
inline constexpr uint32_t forceAmpF32_205=205u;
inline constexpr uint32_t motionMode_206=206u;
inline constexpr uint32_t dedupFlags=207u;
inline constexpr uint32_t sourceSampleNativeNs_lo=208u;
inline constexpr uint32_t sourceSampleNativeNs_hi=209u;
inline constexpr uint32_t acceptedCommandSerial_lo=210u;
inline constexpr uint32_t acceptedCommandSerial_hi=211u;
inline constexpr uint32_t sourceInputSHA_0=212u;
inline constexpr uint32_t sourceInputSHA_1=213u;
inline constexpr uint32_t sourceInputSHA_2=214u;
inline constexpr uint32_t sourceInputSHA_3=215u;
inline constexpr uint32_t sourceInputSHA_4=216u;
inline constexpr uint32_t sourceInputSHA_5=217u;
inline constexpr uint32_t sourceInputSHA_6=218u;
inline constexpr uint32_t sourceInputSHA_7=219u;
inline constexpr uint32_t motionOriginTick_lo=220u;
inline constexpr uint32_t motionOriginTick_hi=221u;
inline constexpr uint32_t controlStatus=222u;
inline constexpr uint32_t reserved0=223u;
inline constexpr uint32_t reserved0_224=224u;
inline constexpr uint32_t reserved0_225=225u;
inline constexpr uint32_t reserved0_226=226u;
inline constexpr uint32_t reserved0_227=227u;
inline constexpr uint32_t reserved0_228=228u;
inline constexpr uint32_t reserved0_229=229u;
inline constexpr uint32_t reserved0_230=230u;
inline constexpr uint32_t reserved0_231=231u;
inline constexpr uint32_t reserved0_232=232u;
inline constexpr uint32_t reserved0_233=233u;
inline constexpr uint32_t reserved0_234=234u;
inline constexpr uint32_t reserved0_235=235u;
inline constexpr uint32_t reserved0_236=236u;
inline constexpr uint32_t reserved0_237=237u;
inline constexpr uint32_t reserved0_238=238u;
inline constexpr uint32_t reserved0_239=239u;
inline constexpr uint32_t reserved0_240=240u;
inline constexpr uint32_t reserved0_241=241u;
inline constexpr uint32_t reserved0_242=242u;
inline constexpr uint32_t reserved0_243=243u;
inline constexpr uint32_t reserved0_244=244u;
inline constexpr uint32_t reserved0_245=245u;
inline constexpr uint32_t reserved0_246=246u;
inline constexpr uint32_t reserved0_247=247u;
inline constexpr uint32_t reserved0_248=248u;
inline constexpr uint32_t reserved0_249=249u;
inline constexpr uint32_t reserved0_250=250u;
inline constexpr uint32_t reserved0_251=251u;
inline constexpr uint32_t reserved0_252=252u;
inline constexpr uint32_t reserved0_253=253u;
inline constexpr uint32_t reserved0_254=254u;
inline constexpr uint32_t reserved0_255=255u;
}
namespace Input128 {
inline constexpr uint32_t magic=0u;
inline constexpr uint32_t schema=1u;
inline constexpr uint32_t lengthWords=2u;
inline constexpr uint32_t kind=3u;
inline constexpr uint32_t run_lo=4u;
inline constexpr uint32_t run_hi=5u;
inline constexpr uint32_t context_lo=6u;
inline constexpr uint32_t context_hi=7u;
inline constexpr uint32_t surface_lo=8u;
inline constexpr uint32_t surface_hi=9u;
inline constexpr uint32_t geometry_lo=10u;
inline constexpr uint32_t geometry_hi=11u;
inline constexpr uint32_t configGeneration_lo=12u;
inline constexpr uint32_t configGeneration_hi=13u;
inline constexpr uint32_t pageSession_lo=14u;
inline constexpr uint32_t pageSession_hi=15u;
inline constexpr uint32_t permission_lo=16u;
inline constexpr uint32_t permission_hi=17u;
inline constexpr uint32_t controlGeneration_lo=18u;
inline constexpr uint32_t controlGeneration_hi=19u;
inline constexpr uint32_t commandSeq_lo=20u;
inline constexpr uint32_t commandSeq_hi=21u;
inline constexpr uint32_t sampleNativeNs_lo=22u;
inline constexpr uint32_t sampleNativeNs_hi=23u;
inline constexpr uint32_t baseCommit_lo=24u;
inline constexpr uint32_t baseCommit_hi=25u;
inline constexpr uint32_t acceptedQuantum_lo=26u;
inline constexpr uint32_t acceptedQuantum_hi=27u;
inline constexpr uint32_t motionOriginQuantum_lo=28u;
inline constexpr uint32_t motionOriginQuantum_hi=29u;
inline constexpr uint32_t diagnosticMode=30u;
inline constexpr uint32_t forcedJ=31u;
inline constexpr uint32_t configSHA_0=32u;
inline constexpr uint32_t configSHA_1=33u;
inline constexpr uint32_t configSHA_2=34u;
inline constexpr uint32_t configSHA_3=35u;
inline constexpr uint32_t configSHA_4=36u;
inline constexpr uint32_t configSHA_5=37u;
inline constexpr uint32_t configSHA_6=38u;
inline constexpr uint32_t configSHA_7=39u;
inline constexpr uint32_t algorithmSHA_0=40u;
inline constexpr uint32_t algorithmSHA_1=41u;
inline constexpr uint32_t algorithmSHA_2=42u;
inline constexpr uint32_t algorithmSHA_3=43u;
inline constexpr uint32_t algorithmSHA_4=44u;
inline constexpr uint32_t algorithmSHA_5=45u;
inline constexpr uint32_t algorithmSHA_6=46u;
inline constexpr uint32_t algorithmSHA_7=47u;
inline constexpr uint32_t layoutSHA_0=48u;
inline constexpr uint32_t layoutSHA_1=49u;
inline constexpr uint32_t layoutSHA_2=50u;
inline constexpr uint32_t layoutSHA_3=51u;
inline constexpr uint32_t layoutSHA_4=52u;
inline constexpr uint32_t layoutSHA_5=53u;
inline constexpr uint32_t layoutSHA_6=54u;
inline constexpr uint32_t layoutSHA_7=55u;
inline constexpr uint32_t gatesSHA_0=56u;
inline constexpr uint32_t gatesSHA_1=57u;
inline constexpr uint32_t gatesSHA_2=58u;
inline constexpr uint32_t gatesSHA_3=59u;
inline constexpr uint32_t gatesSHA_4=60u;
inline constexpr uint32_t gatesSHA_5=61u;
inline constexpr uint32_t gatesSHA_6=62u;
inline constexpr uint32_t gatesSHA_7=63u;
inline constexpr uint32_t seedSHA_0=64u;
inline constexpr uint32_t seedSHA_1=65u;
inline constexpr uint32_t seedSHA_2=66u;
inline constexpr uint32_t seedSHA_3=67u;
inline constexpr uint32_t seedSHA_4=68u;
inline constexpr uint32_t seedSHA_5=69u;
inline constexpr uint32_t seedSHA_6=70u;
inline constexpr uint32_t seedSHA_7=71u;
inline constexpr uint32_t fixtureSHA_0=72u;
inline constexpr uint32_t fixtureSHA_1=73u;
inline constexpr uint32_t fixtureSHA_2=74u;
inline constexpr uint32_t fixtureSHA_3=75u;
inline constexpr uint32_t fixtureSHA_4=76u;
inline constexpr uint32_t fixtureSHA_5=77u;
inline constexpr uint32_t fixtureSHA_6=78u;
inline constexpr uint32_t fixtureSHA_7=79u;
inline constexpr uint32_t inputSHA_0=80u;
inline constexpr uint32_t inputSHA_1=81u;
inline constexpr uint32_t inputSHA_2=82u;
inline constexpr uint32_t inputSHA_3=83u;
inline constexpr uint32_t inputSHA_4=84u;
inline constexpr uint32_t inputSHA_5=85u;
inline constexpr uint32_t inputSHA_6=86u;
inline constexpr uint32_t inputSHA_7=87u;
inline constexpr uint32_t sourceOpcode=88u;
inline constexpr uint32_t motionMode=89u;
inline constexpr uint32_t theta0F32=90u;
inline constexpr uint32_t OmF32=91u;
inline constexpr uint32_t durationF32=92u;
inline constexpr uint32_t gxWorldF32=93u;
inline constexpr uint32_t gyWorldF32=94u;
inline constexpr uint32_t forceKind=95u;
inline constexpr uint32_t forceAmpF32=96u;
inline constexpr uint32_t forceStartQuantum_lo=97u;
inline constexpr uint32_t forceStartQuantum_hi=98u;
inline constexpr uint32_t forceEndQuantum_lo=99u;
inline constexpr uint32_t forceEndQuantum_hi=100u;
inline constexpr uint32_t wallBottomF32=101u;
inline constexpr uint32_t wallTopF32=102u;
inline constexpr uint32_t boundaryRevision=103u;
inline constexpr uint32_t sourceIntent_lo=104u;
inline constexpr uint32_t sourceIntent_hi=105u;
inline constexpr uint32_t enqueueSerial_lo=106u;
inline constexpr uint32_t enqueueSerial_hi=107u;
inline constexpr uint32_t reserved0_108=108u;
inline constexpr uint32_t reserved0_109=109u;
inline constexpr uint32_t reserved0_110=110u;
inline constexpr uint32_t reserved0_111=111u;
inline constexpr uint32_t reserved0_112=112u;
inline constexpr uint32_t reserved0_113=113u;
inline constexpr uint32_t reserved0_114=114u;
inline constexpr uint32_t reserved0_115=115u;
inline constexpr uint32_t reserved0_116=116u;
inline constexpr uint32_t reserved0_117=117u;
inline constexpr uint32_t reserved0_118=118u;
inline constexpr uint32_t reserved0_119=119u;
inline constexpr uint32_t reserved0_120=120u;
inline constexpr uint32_t reserved0_121=121u;
inline constexpr uint32_t reserved0_122=122u;
inline constexpr uint32_t reserved0_123=123u;
inline constexpr uint32_t reserved0_124=124u;
inline constexpr uint32_t reserved0_125=125u;
inline constexpr uint32_t reserved0_126=126u;
inline constexpr uint32_t seal=127u;
}
namespace Completion256 {
inline constexpr uint32_t magic=0u;
inline constexpr uint32_t schema=1u;
inline constexpr uint32_t profile=2u;
inline constexpr uint32_t lengthWords=3u;
inline constexpr uint32_t rootKind=4u;
inline constexpr uint32_t outcome=5u;
inline constexpr uint32_t firstErrorOp=6u;
inline constexpr uint32_t firstError=7u;
inline constexpr uint32_t nonce_lo=8u;
inline constexpr uint32_t nonce_hi=9u;
inline constexpr uint32_t plan_lo=10u;
inline constexpr uint32_t plan_hi=11u;
inline constexpr uint32_t run_lo=12u;
inline constexpr uint32_t run_hi=13u;
inline constexpr uint32_t owner_lo=14u;
inline constexpr uint32_t owner_hi=15u;
inline constexpr uint32_t context_lo=16u;
inline constexpr uint32_t context_hi=17u;
inline constexpr uint32_t surface_lo=18u;
inline constexpr uint32_t surface_hi=19u;
inline constexpr uint32_t geometry_lo=20u;
inline constexpr uint32_t geometry_hi=21u;
inline constexpr uint32_t configGeneration_lo=22u;
inline constexpr uint32_t configGeneration_hi=23u;
inline constexpr uint32_t baseCommit_lo=24u;
inline constexpr uint32_t baseCommit_hi=25u;
inline constexpr uint32_t proposedCommit_lo=26u;
inline constexpr uint32_t proposedCommit_hi=27u;
inline constexpr uint32_t startedNativeNs_lo=28u;
inline constexpr uint32_t startedNativeNs_hi=29u;
inline constexpr uint32_t deadlineNativeNs_lo=30u;
inline constexpr uint32_t deadlineNativeNs_hi=31u;
inline constexpr uint32_t completedNativeNs_lo=32u;
inline constexpr uint32_t completedNativeNs_hi=33u;
inline constexpr uint32_t acceptedQuantum_lo=34u;
inline constexpr uint32_t acceptedQuantum_hi=35u;
inline constexpr uint32_t selectedJ=36u;
inline constexpr uint32_t requiredAuditMask=37u;
inline constexpr uint32_t actualAuditMask=38u;
inline constexpr uint32_t actualDispatches=39u;
inline constexpr uint32_t activeDispatches=40u;
inline constexpr uint32_t noopDispatches=41u;
inline constexpr uint32_t publicationEligible=42u;
inline constexpr uint32_t traceSealedMask=43u;
inline constexpr uint32_t consumedSourceSeq_lo=44u;
inline constexpr uint32_t consumedSourceSeq_hi=45u;
inline constexpr uint32_t sourceGeneration_lo=46u;
inline constexpr uint32_t sourceGeneration_hi=47u;
inline constexpr uint32_t configSHA_0=48u;
inline constexpr uint32_t configSHA_1=49u;
inline constexpr uint32_t configSHA_2=50u;
inline constexpr uint32_t configSHA_3=51u;
inline constexpr uint32_t configSHA_4=52u;
inline constexpr uint32_t configSHA_5=53u;
inline constexpr uint32_t configSHA_6=54u;
inline constexpr uint32_t configSHA_7=55u;
inline constexpr uint32_t algorithmSHA_0=56u;
inline constexpr uint32_t algorithmSHA_1=57u;
inline constexpr uint32_t algorithmSHA_2=58u;
inline constexpr uint32_t algorithmSHA_3=59u;
inline constexpr uint32_t algorithmSHA_4=60u;
inline constexpr uint32_t algorithmSHA_5=61u;
inline constexpr uint32_t algorithmSHA_6=62u;
inline constexpr uint32_t algorithmSHA_7=63u;
inline constexpr uint32_t layoutSHA_0=64u;
inline constexpr uint32_t layoutSHA_1=65u;
inline constexpr uint32_t layoutSHA_2=66u;
inline constexpr uint32_t layoutSHA_3=67u;
inline constexpr uint32_t layoutSHA_4=68u;
inline constexpr uint32_t layoutSHA_5=69u;
inline constexpr uint32_t layoutSHA_6=70u;
inline constexpr uint32_t layoutSHA_7=71u;
inline constexpr uint32_t gatesSHA_0=72u;
inline constexpr uint32_t gatesSHA_1=73u;
inline constexpr uint32_t gatesSHA_2=74u;
inline constexpr uint32_t gatesSHA_3=75u;
inline constexpr uint32_t gatesSHA_4=76u;
inline constexpr uint32_t gatesSHA_5=77u;
inline constexpr uint32_t gatesSHA_6=78u;
inline constexpr uint32_t gatesSHA_7=79u;
inline constexpr uint32_t seedSHA_0=80u;
inline constexpr uint32_t seedSHA_1=81u;
inline constexpr uint32_t seedSHA_2=82u;
inline constexpr uint32_t seedSHA_3=83u;
inline constexpr uint32_t seedSHA_4=84u;
inline constexpr uint32_t seedSHA_5=85u;
inline constexpr uint32_t seedSHA_6=86u;
inline constexpr uint32_t seedSHA_7=87u;
inline constexpr uint32_t inputSHA_0=88u;
inline constexpr uint32_t inputSHA_1=89u;
inline constexpr uint32_t inputSHA_2=90u;
inline constexpr uint32_t inputSHA_3=91u;
inline constexpr uint32_t inputSHA_4=92u;
inline constexpr uint32_t inputSHA_5=93u;
inline constexpr uint32_t inputSHA_6=94u;
inline constexpr uint32_t inputSHA_7=95u;
inline constexpr uint32_t fieldSHA_0=96u;
inline constexpr uint32_t fieldSHA_1=97u;
inline constexpr uint32_t fieldSHA_2=98u;
inline constexpr uint32_t fieldSHA_3=99u;
inline constexpr uint32_t fieldSHA_4=100u;
inline constexpr uint32_t fieldSHA_5=101u;
inline constexpr uint32_t fieldSHA_6=102u;
inline constexpr uint32_t fieldSHA_7=103u;
inline constexpr uint32_t rHRMSF32=104u;
inline constexpr uint32_t rHMaxF32=105u;
inline constexpr uint32_t divergenceRMSF32=106u;
inline constexpr uint32_t divergenceMaxF32=107u;
inline constexpr uint32_t pressureCertificateF32=108u;
inline constexpr uint32_t velocityCertificateF32=109u;
inline constexpr uint32_t projectionIdentityMaxF32=110u;
inline constexpr uint32_t RfloatF32=111u;
inline constexpr uint32_t energyBudgetF32=112u;
inline constexpr uint32_t momentumClosureXF32=113u;
inline constexpr uint32_t momentumClosureYF32=114u;
inline constexpr uint32_t positiveWorkF32=115u;
inline constexpr uint32_t renderGeneration_lo=116u;
inline constexpr uint32_t renderGeneration_hi=117u;
inline constexpr uint32_t rootTraceGeneration_lo=118u;
inline constexpr uint32_t rootTraceGeneration_hi=119u;
inline constexpr uint32_t candidateBank=120u;
inline constexpr uint32_t renderPackBank=121u;
inline constexpr uint32_t failureScope=122u;
inline constexpr uint32_t failureTrial=123u;
inline constexpr uint32_t failureMicro=124u;
inline constexpr uint32_t mathBackend=125u;
inline constexpr uint32_t diagnosticMode=126u;
inline constexpr uint32_t precisionFlags=127u;
inline constexpr uint32_t pageSession_lo=128u;
inline constexpr uint32_t pageSession_hi=129u;
inline constexpr uint32_t permission_lo=130u;
inline constexpr uint32_t permission_hi=131u;
inline constexpr uint32_t inputEpoch_lo=132u;
inline constexpr uint32_t inputEpoch_hi=133u;
inline constexpr uint32_t controlGeneration_lo=134u;
inline constexpr uint32_t controlGeneration_hi=135u;
inline constexpr uint32_t thetaWrappedF32=136u;
inline constexpr uint32_t OmegaF32=137u;
inline constexpr uint32_t alphaF32=138u;
inline constexpr uint32_t gxBoxF32=139u;
inline constexpr uint32_t gyBoxF32=140u;
inline constexpr uint32_t motionEnded=141u;
inline constexpr uint32_t forceOn=142u;
inline constexpr uint32_t sourceControlValid=143u;
inline constexpr uint32_t motionOriginQuantum_lo=144u;
inline constexpr uint32_t motionOriginQuantum_hi=145u;
inline constexpr uint32_t reserved0_146=146u;
inline constexpr uint32_t reserved0_147=147u;
inline constexpr uint32_t reserved0_148=148u;
inline constexpr uint32_t reserved0_149=149u;
inline constexpr uint32_t reserved0_150=150u;
inline constexpr uint32_t reserved0_151=151u;
inline constexpr uint32_t reserved0_152=152u;
inline constexpr uint32_t reserved0_153=153u;
inline constexpr uint32_t reserved0_154=154u;
inline constexpr uint32_t reserved0_155=155u;
inline constexpr uint32_t reserved0_156=156u;
inline constexpr uint32_t reserved0_157=157u;
inline constexpr uint32_t reserved0_158=158u;
inline constexpr uint32_t reserved0_159=159u;
inline constexpr uint32_t reserved0_160=160u;
inline constexpr uint32_t reserved0_161=161u;
inline constexpr uint32_t reserved0_162=162u;
inline constexpr uint32_t reserved0_163=163u;
inline constexpr uint32_t reserved0_164=164u;
inline constexpr uint32_t reserved0_165=165u;
inline constexpr uint32_t reserved0_166=166u;
inline constexpr uint32_t reserved0_167=167u;
inline constexpr uint32_t reserved0_168=168u;
inline constexpr uint32_t reserved0_169=169u;
inline constexpr uint32_t reserved0_170=170u;
inline constexpr uint32_t reserved0_171=171u;
inline constexpr uint32_t reserved0_172=172u;
inline constexpr uint32_t reserved0_173=173u;
inline constexpr uint32_t reserved0_174=174u;
inline constexpr uint32_t reserved0_175=175u;
inline constexpr uint32_t reserved0_176=176u;
inline constexpr uint32_t reserved0_177=177u;
inline constexpr uint32_t reserved0_178=178u;
inline constexpr uint32_t reserved0_179=179u;
inline constexpr uint32_t reserved0_180=180u;
inline constexpr uint32_t reserved0_181=181u;
inline constexpr uint32_t reserved0_182=182u;
inline constexpr uint32_t reserved0_183=183u;
inline constexpr uint32_t reserved0_184=184u;
inline constexpr uint32_t reserved0_185=185u;
inline constexpr uint32_t reserved0_186=186u;
inline constexpr uint32_t reserved0_187=187u;
inline constexpr uint32_t reserved0_188=188u;
inline constexpr uint32_t reserved0_189=189u;
inline constexpr uint32_t reserved0_190=190u;
inline constexpr uint32_t reserved0_191=191u;
inline constexpr uint32_t reserved0_192=192u;
inline constexpr uint32_t reserved0_193=193u;
inline constexpr uint32_t reserved0_194=194u;
inline constexpr uint32_t reserved0_195=195u;
inline constexpr uint32_t reserved0_196=196u;
inline constexpr uint32_t reserved0_197=197u;
inline constexpr uint32_t reserved0_198=198u;
inline constexpr uint32_t reserved0_199=199u;
inline constexpr uint32_t reserved0_200=200u;
inline constexpr uint32_t reserved0_201=201u;
inline constexpr uint32_t reserved0_202=202u;
inline constexpr uint32_t reserved0_203=203u;
inline constexpr uint32_t reserved0_204=204u;
inline constexpr uint32_t reserved0_205=205u;
inline constexpr uint32_t reserved0_206=206u;
inline constexpr uint32_t reserved0_207=207u;
inline constexpr uint32_t reserved0_208=208u;
inline constexpr uint32_t reserved0_209=209u;
inline constexpr uint32_t reserved0_210=210u;
inline constexpr uint32_t reserved0_211=211u;
inline constexpr uint32_t reserved0_212=212u;
inline constexpr uint32_t reserved0_213=213u;
inline constexpr uint32_t reserved0_214=214u;
inline constexpr uint32_t reserved0_215=215u;
inline constexpr uint32_t reserved0_216=216u;
inline constexpr uint32_t reserved0_217=217u;
inline constexpr uint32_t reserved0_218=218u;
inline constexpr uint32_t reserved0_219=219u;
inline constexpr uint32_t reserved0_220=220u;
inline constexpr uint32_t reserved0_221=221u;
inline constexpr uint32_t reserved0_222=222u;
inline constexpr uint32_t reserved0_223=223u;
inline constexpr uint32_t reserved0_224=224u;
inline constexpr uint32_t reserved0_225=225u;
inline constexpr uint32_t reserved0_226=226u;
inline constexpr uint32_t reserved0_227=227u;
inline constexpr uint32_t reserved0_228=228u;
inline constexpr uint32_t reserved0_229=229u;
inline constexpr uint32_t reserved0_230=230u;
inline constexpr uint32_t reserved0_231=231u;
inline constexpr uint32_t reserved0_232=232u;
inline constexpr uint32_t reserved0_233=233u;
inline constexpr uint32_t reserved0_234=234u;
inline constexpr uint32_t reserved0_235=235u;
inline constexpr uint32_t reserved0_236=236u;
inline constexpr uint32_t reserved0_237=237u;
inline constexpr uint32_t reserved0_238=238u;
inline constexpr uint32_t reserved0_239=239u;
inline constexpr uint32_t reserved0_240=240u;
inline constexpr uint32_t reserved0_241=241u;
inline constexpr uint32_t reserved0_242=242u;
inline constexpr uint32_t reserved0_243=243u;
inline constexpr uint32_t reserved0_244=244u;
inline constexpr uint32_t reserved0_245=245u;
inline constexpr uint32_t reserved0_246=246u;
inline constexpr uint32_t reserved0_247=247u;
inline constexpr uint32_t reserved0_248=248u;
inline constexpr uint32_t reserved0_249=249u;
inline constexpr uint32_t reserved0_250=250u;
inline constexpr uint32_t reserved0_251=251u;
inline constexpr uint32_t reserved0_252=252u;
inline constexpr uint32_t reserved0_253=253u;
inline constexpr uint32_t reserved0_254=254u;
inline constexpr uint32_t seal=255u;
}
namespace MicroTrace1024 {
inline constexpr uint32_t magic=0u;
inline constexpr uint32_t schema=1u;
inline constexpr uint32_t profile=2u;
inline constexpr uint32_t lengthWords=3u;
inline constexpr uint32_t scope=4u;
inline constexpr uint32_t trialJ=5u;
inline constexpr uint32_t microOrdinal=6u;
inline constexpr uint32_t status=7u;
inline constexpr uint32_t nonce_lo=8u;
inline constexpr uint32_t nonce_hi=9u;
inline constexpr uint32_t plan_lo=10u;
inline constexpr uint32_t plan_hi=11u;
inline constexpr uint32_t baseCommit_lo=12u;
inline constexpr uint32_t baseCommit_hi=13u;
inline constexpr uint32_t acceptedTickBefore_lo=14u;
inline constexpr uint32_t acceptedTickBefore_hi=15u;
inline constexpr uint32_t acceptedTickAfter_lo=16u;
inline constexpr uint32_t acceptedTickAfter_hi=17u;
inline constexpr uint32_t sourceSeq_lo=18u;
inline constexpr uint32_t sourceSeq_hi=19u;
inline constexpr uint32_t deltaDenominator=20u;
inline constexpr uint32_t firstErrorOp=21u;
inline constexpr uint32_t firstError=22u;
inline constexpr uint32_t completedFixedPassMask=23u;
inline constexpr uint32_t reserved0_24=24u;
inline constexpr uint32_t reserved0_25=25u;
inline constexpr uint32_t reserved0_26=26u;
inline constexpr uint32_t reserved0_27_27=27u;
inline constexpr uint32_t reserved0_28_28=28u;
inline constexpr uint32_t reserved0_29_29=29u;
inline constexpr uint32_t reserved0_30_30=30u;
inline constexpr uint32_t reserved0_31_31=31u;
inline constexpr uint32_t inputSHA_0=32u;
inline constexpr uint32_t inputSHA_1=33u;
inline constexpr uint32_t inputSHA_2=34u;
inline constexpr uint32_t inputSHA_3=35u;
inline constexpr uint32_t inputSHA_4=36u;
inline constexpr uint32_t inputSHA_5=37u;
inline constexpr uint32_t inputSHA_6=38u;
inline constexpr uint32_t inputSHA_7=39u;
inline constexpr uint32_t configSHA_0=40u;
inline constexpr uint32_t configSHA_1=41u;
inline constexpr uint32_t configSHA_2=42u;
inline constexpr uint32_t configSHA_3=43u;
inline constexpr uint32_t configSHA_4=44u;
inline constexpr uint32_t configSHA_5=45u;
inline constexpr uint32_t configSHA_6=46u;
inline constexpr uint32_t configSHA_7=47u;
inline constexpr uint32_t algorithmSHA_0=48u;
inline constexpr uint32_t algorithmSHA_1=49u;
inline constexpr uint32_t algorithmSHA_2=50u;
inline constexpr uint32_t algorithmSHA_3=51u;
inline constexpr uint32_t algorithmSHA_4=52u;
inline constexpr uint32_t algorithmSHA_5=53u;
inline constexpr uint32_t algorithmSHA_6=54u;
inline constexpr uint32_t algorithmSHA_7=55u;
inline constexpr uint32_t layoutSHA_0=56u;
inline constexpr uint32_t layoutSHA_1=57u;
inline constexpr uint32_t layoutSHA_2=58u;
inline constexpr uint32_t layoutSHA_3=59u;
inline constexpr uint32_t layoutSHA_4=60u;
inline constexpr uint32_t layoutSHA_5=61u;
inline constexpr uint32_t layoutSHA_6=62u;
inline constexpr uint32_t layoutSHA_7=63u;
inline constexpr uint32_t gatesSHA_0=64u;
inline constexpr uint32_t gatesSHA_1=65u;
inline constexpr uint32_t gatesSHA_2=66u;
inline constexpr uint32_t gatesSHA_3=67u;
inline constexpr uint32_t gatesSHA_4=68u;
inline constexpr uint32_t gatesSHA_5=69u;
inline constexpr uint32_t gatesSHA_6=70u;
inline constexpr uint32_t gatesSHA_7=71u;
inline constexpr uint32_t thetaF32=72u;
inline constexpr uint32_t OmegaF32_73=73u;
inline constexpr uint32_t alphaF32_74=74u;
inline constexpr uint32_t gxF32_75=75u;
inline constexpr uint32_t gyF32_76=76u;
inline constexpr uint32_t forceStartFlag=77u;
inline constexpr uint32_t positiveWorkF32=78u;
inline constexpr uint32_t CFLincomingF32=79u;
inline constexpr uint32_t CFLoutgoingF32=80u;
inline constexpr uint32_t uMaxF32=81u;
inline constexpr uint32_t vMaxF32=82u;
inline constexpr uint32_t hF32_83=83u;
inline constexpr uint32_t nuF32_84=84u;
inline constexpr uint32_t lambdaLowerBoundF32_85=85u;
inline constexpr uint32_t etaBF32_86=86u;
inline constexpr uint32_t mathBackend=87u;
inline constexpr uint32_t reserved0_88=88u;
inline constexpr uint32_t reserved0_89=89u;
inline constexpr uint32_t reserved0_90=90u;
inline constexpr uint32_t reserved0_91=91u;
inline constexpr uint32_t reserved0_92=92u;
inline constexpr uint32_t reserved0_93=93u;
inline constexpr uint32_t reserved0_94=94u;
inline constexpr uint32_t reserved0_95=95u;
inline constexpr uint32_t reserved0_96=96u;
inline constexpr uint32_t reserved0_97=97u;
inline constexpr uint32_t reserved0_98=98u;
inline constexpr uint32_t reserved0_99=99u;
inline constexpr uint32_t reserved0_100=100u;
inline constexpr uint32_t reserved0_101=101u;
inline constexpr uint32_t reserved0_102=102u;
inline constexpr uint32_t reserved0_103=103u;
inline constexpr uint32_t reserved0_104=104u;
inline constexpr uint32_t reserved0_105=105u;
inline constexpr uint32_t reserved0_106=106u;
inline constexpr uint32_t reserved0_107=107u;
inline constexpr uint32_t reserved0_108=108u;
inline constexpr uint32_t reserved0_109=109u;
inline constexpr uint32_t reserved0_110=110u;
inline constexpr uint32_t reserved0_111=111u;
inline constexpr uint32_t reserved0_112=112u;
inline constexpr uint32_t reserved0_113=113u;
inline constexpr uint32_t reserved0_114=114u;
inline constexpr uint32_t reserved0_115=115u;
inline constexpr uint32_t reserved0_116=116u;
inline constexpr uint32_t reserved0_117=117u;
inline constexpr uint32_t reserved0_118=118u;
inline constexpr uint32_t reserved0_119=119u;
inline constexpr uint32_t reserved0_120=120u;
inline constexpr uint32_t reserved0_121=121u;
inline constexpr uint32_t reserved0_122=122u;
inline constexpr uint32_t reserved0_123=123u;
inline constexpr uint32_t reserved0_124=124u;
inline constexpr uint32_t reserved0_125=125u;
inline constexpr uint32_t reserved0_126=126u;
inline constexpr uint32_t reserved0_127=127u;
inline constexpr uint32_t pressureCycle0_ordinal=128u;
inline constexpr uint32_t pressureCycle0_flags=129u;
inline constexpr uint32_t pressureCycle0_converged=130u;
inline constexpr uint32_t pressureCycle0_fault=131u;
inline constexpr uint32_t pressureCycle0_rSquared0=132u;
inline constexpr uint32_t pressureCycle0_rSquared1=133u;
inline constexpr uint32_t pressureCycle0_rSquared2=134u;
inline constexpr uint32_t pressureCycle0_rSquared3=135u;
inline constexpr uint32_t pressureCycle0_rMaxLo=136u;
inline constexpr uint32_t pressureCycle0_rMaxHi=137u;
inline constexpr uint32_t pressureCycle0_sumBLo=138u;
inline constexpr uint32_t pressureCycle0_sumBHi=139u;
inline constexpr uint32_t pressureCycle0_gaugeMeanLo=140u;
inline constexpr uint32_t pressureCycle0_gaugeMeanHi=141u;
inline constexpr uint32_t pressureCycle0_activeSweeps=142u;
inline constexpr uint32_t pressureCycle0_seal=143u;
inline constexpr uint32_t pressureCycle1_ordinal=144u;
inline constexpr uint32_t pressureCycle1_flags=145u;
inline constexpr uint32_t pressureCycle1_converged=146u;
inline constexpr uint32_t pressureCycle1_fault=147u;
inline constexpr uint32_t pressureCycle1_rSquared0=148u;
inline constexpr uint32_t pressureCycle1_rSquared1=149u;
inline constexpr uint32_t pressureCycle1_rSquared2=150u;
inline constexpr uint32_t pressureCycle1_rSquared3=151u;
inline constexpr uint32_t pressureCycle1_rMaxLo=152u;
inline constexpr uint32_t pressureCycle1_rMaxHi=153u;
inline constexpr uint32_t pressureCycle1_sumBLo=154u;
inline constexpr uint32_t pressureCycle1_sumBHi=155u;
inline constexpr uint32_t pressureCycle1_gaugeMeanLo=156u;
inline constexpr uint32_t pressureCycle1_gaugeMeanHi=157u;
inline constexpr uint32_t pressureCycle1_activeSweeps=158u;
inline constexpr uint32_t pressureCycle1_seal=159u;
inline constexpr uint32_t pressureCycle2_ordinal=160u;
inline constexpr uint32_t pressureCycle2_flags=161u;
inline constexpr uint32_t pressureCycle2_converged=162u;
inline constexpr uint32_t pressureCycle2_fault=163u;
inline constexpr uint32_t pressureCycle2_rSquared0=164u;
inline constexpr uint32_t pressureCycle2_rSquared1=165u;
inline constexpr uint32_t pressureCycle2_rSquared2=166u;
inline constexpr uint32_t pressureCycle2_rSquared3=167u;
inline constexpr uint32_t pressureCycle2_rMaxLo=168u;
inline constexpr uint32_t pressureCycle2_rMaxHi=169u;
inline constexpr uint32_t pressureCycle2_sumBLo=170u;
inline constexpr uint32_t pressureCycle2_sumBHi=171u;
inline constexpr uint32_t pressureCycle2_gaugeMeanLo=172u;
inline constexpr uint32_t pressureCycle2_gaugeMeanHi=173u;
inline constexpr uint32_t pressureCycle2_activeSweeps=174u;
inline constexpr uint32_t pressureCycle2_seal=175u;
inline constexpr uint32_t pressureCycle3_ordinal=176u;
inline constexpr uint32_t pressureCycle3_flags=177u;
inline constexpr uint32_t pressureCycle3_converged=178u;
inline constexpr uint32_t pressureCycle3_fault=179u;
inline constexpr uint32_t pressureCycle3_rSquared0=180u;
inline constexpr uint32_t pressureCycle3_rSquared1=181u;
inline constexpr uint32_t pressureCycle3_rSquared2=182u;
inline constexpr uint32_t pressureCycle3_rSquared3=183u;
inline constexpr uint32_t pressureCycle3_rMaxLo=184u;
inline constexpr uint32_t pressureCycle3_rMaxHi=185u;
inline constexpr uint32_t pressureCycle3_sumBLo=186u;
inline constexpr uint32_t pressureCycle3_sumBHi=187u;
inline constexpr uint32_t pressureCycle3_gaugeMeanLo=188u;
inline constexpr uint32_t pressureCycle3_gaugeMeanHi=189u;
inline constexpr uint32_t pressureCycle3_activeSweeps=190u;
inline constexpr uint32_t pressureCycle3_seal=191u;
inline constexpr uint32_t pressureCycle4_ordinal=192u;
inline constexpr uint32_t pressureCycle4_flags=193u;
inline constexpr uint32_t pressureCycle4_converged=194u;
inline constexpr uint32_t pressureCycle4_fault=195u;
inline constexpr uint32_t pressureCycle4_rSquared0=196u;
inline constexpr uint32_t pressureCycle4_rSquared1=197u;
inline constexpr uint32_t pressureCycle4_rSquared2=198u;
inline constexpr uint32_t pressureCycle4_rSquared3=199u;
inline constexpr uint32_t pressureCycle4_rMaxLo=200u;
inline constexpr uint32_t pressureCycle4_rMaxHi=201u;
inline constexpr uint32_t pressureCycle4_sumBLo=202u;
inline constexpr uint32_t pressureCycle4_sumBHi=203u;
inline constexpr uint32_t pressureCycle4_gaugeMeanLo=204u;
inline constexpr uint32_t pressureCycle4_gaugeMeanHi=205u;
inline constexpr uint32_t pressureCycle4_activeSweeps=206u;
inline constexpr uint32_t pressureCycle4_seal=207u;
inline constexpr uint32_t pressureCycle5_ordinal=208u;
inline constexpr uint32_t pressureCycle5_flags=209u;
inline constexpr uint32_t pressureCycle5_converged=210u;
inline constexpr uint32_t pressureCycle5_fault=211u;
inline constexpr uint32_t pressureCycle5_rSquared0=212u;
inline constexpr uint32_t pressureCycle5_rSquared1=213u;
inline constexpr uint32_t pressureCycle5_rSquared2=214u;
inline constexpr uint32_t pressureCycle5_rSquared3=215u;
inline constexpr uint32_t pressureCycle5_rMaxLo=216u;
inline constexpr uint32_t pressureCycle5_rMaxHi=217u;
inline constexpr uint32_t pressureCycle5_sumBLo=218u;
inline constexpr uint32_t pressureCycle5_sumBHi=219u;
inline constexpr uint32_t pressureCycle5_gaugeMeanLo=220u;
inline constexpr uint32_t pressureCycle5_gaugeMeanHi=221u;
inline constexpr uint32_t pressureCycle5_activeSweeps=222u;
inline constexpr uint32_t pressureCycle5_seal=223u;
inline constexpr uint32_t pressureCycle6_ordinal=224u;
inline constexpr uint32_t pressureCycle6_flags=225u;
inline constexpr uint32_t pressureCycle6_converged=226u;
inline constexpr uint32_t pressureCycle6_fault=227u;
inline constexpr uint32_t pressureCycle6_rSquared0=228u;
inline constexpr uint32_t pressureCycle6_rSquared1=229u;
inline constexpr uint32_t pressureCycle6_rSquared2=230u;
inline constexpr uint32_t pressureCycle6_rSquared3=231u;
inline constexpr uint32_t pressureCycle6_rMaxLo=232u;
inline constexpr uint32_t pressureCycle6_rMaxHi=233u;
inline constexpr uint32_t pressureCycle6_sumBLo=234u;
inline constexpr uint32_t pressureCycle6_sumBHi=235u;
inline constexpr uint32_t pressureCycle6_gaugeMeanLo=236u;
inline constexpr uint32_t pressureCycle6_gaugeMeanHi=237u;
inline constexpr uint32_t pressureCycle6_activeSweeps=238u;
inline constexpr uint32_t pressureCycle6_seal=239u;
inline constexpr uint32_t pressureCycle7_ordinal=240u;
inline constexpr uint32_t pressureCycle7_flags=241u;
inline constexpr uint32_t pressureCycle7_converged=242u;
inline constexpr uint32_t pressureCycle7_fault=243u;
inline constexpr uint32_t pressureCycle7_rSquared0=244u;
inline constexpr uint32_t pressureCycle7_rSquared1=245u;
inline constexpr uint32_t pressureCycle7_rSquared2=246u;
inline constexpr uint32_t pressureCycle7_rSquared3=247u;
inline constexpr uint32_t pressureCycle7_rMaxLo=248u;
inline constexpr uint32_t pressureCycle7_rMaxHi=249u;
inline constexpr uint32_t pressureCycle7_sumBLo=250u;
inline constexpr uint32_t pressureCycle7_sumBHi=251u;
inline constexpr uint32_t pressureCycle7_gaugeMeanLo=252u;
inline constexpr uint32_t pressureCycle7_gaugeMeanHi=253u;
inline constexpr uint32_t pressureCycle7_activeSweeps=254u;
inline constexpr uint32_t pressureCycle7_seal=255u;
inline constexpr uint32_t pressureCycle8_ordinal=256u;
inline constexpr uint32_t pressureCycle8_flags=257u;
inline constexpr uint32_t pressureCycle8_converged=258u;
inline constexpr uint32_t pressureCycle8_fault=259u;
inline constexpr uint32_t pressureCycle8_rSquared0=260u;
inline constexpr uint32_t pressureCycle8_rSquared1=261u;
inline constexpr uint32_t pressureCycle8_rSquared2=262u;
inline constexpr uint32_t pressureCycle8_rSquared3=263u;
inline constexpr uint32_t pressureCycle8_rMaxLo=264u;
inline constexpr uint32_t pressureCycle8_rMaxHi=265u;
inline constexpr uint32_t pressureCycle8_sumBLo=266u;
inline constexpr uint32_t pressureCycle8_sumBHi=267u;
inline constexpr uint32_t pressureCycle8_gaugeMeanLo=268u;
inline constexpr uint32_t pressureCycle8_gaugeMeanHi=269u;
inline constexpr uint32_t pressureCycle8_activeSweeps=270u;
inline constexpr uint32_t pressureCycle8_seal=271u;
inline constexpr uint32_t pressureCycle9_ordinal=272u;
inline constexpr uint32_t pressureCycle9_flags=273u;
inline constexpr uint32_t pressureCycle9_converged=274u;
inline constexpr uint32_t pressureCycle9_fault=275u;
inline constexpr uint32_t pressureCycle9_rSquared0=276u;
inline constexpr uint32_t pressureCycle9_rSquared1=277u;
inline constexpr uint32_t pressureCycle9_rSquared2=278u;
inline constexpr uint32_t pressureCycle9_rSquared3=279u;
inline constexpr uint32_t pressureCycle9_rMaxLo=280u;
inline constexpr uint32_t pressureCycle9_rMaxHi=281u;
inline constexpr uint32_t pressureCycle9_sumBLo=282u;
inline constexpr uint32_t pressureCycle9_sumBHi=283u;
inline constexpr uint32_t pressureCycle9_gaugeMeanLo=284u;
inline constexpr uint32_t pressureCycle9_gaugeMeanHi=285u;
inline constexpr uint32_t pressureCycle9_activeSweeps=286u;
inline constexpr uint32_t pressureCycle9_seal=287u;
inline constexpr uint32_t pressureCycle10_ordinal=288u;
inline constexpr uint32_t pressureCycle10_flags=289u;
inline constexpr uint32_t pressureCycle10_converged=290u;
inline constexpr uint32_t pressureCycle10_fault=291u;
inline constexpr uint32_t pressureCycle10_rSquared0=292u;
inline constexpr uint32_t pressureCycle10_rSquared1=293u;
inline constexpr uint32_t pressureCycle10_rSquared2=294u;
inline constexpr uint32_t pressureCycle10_rSquared3=295u;
inline constexpr uint32_t pressureCycle10_rMaxLo=296u;
inline constexpr uint32_t pressureCycle10_rMaxHi=297u;
inline constexpr uint32_t pressureCycle10_sumBLo=298u;
inline constexpr uint32_t pressureCycle10_sumBHi=299u;
inline constexpr uint32_t pressureCycle10_gaugeMeanLo=300u;
inline constexpr uint32_t pressureCycle10_gaugeMeanHi=301u;
inline constexpr uint32_t pressureCycle10_activeSweeps=302u;
inline constexpr uint32_t pressureCycle10_seal=303u;
inline constexpr uint32_t pressureCycle11_ordinal=304u;
inline constexpr uint32_t pressureCycle11_flags=305u;
inline constexpr uint32_t pressureCycle11_converged=306u;
inline constexpr uint32_t pressureCycle11_fault=307u;
inline constexpr uint32_t pressureCycle11_rSquared0=308u;
inline constexpr uint32_t pressureCycle11_rSquared1=309u;
inline constexpr uint32_t pressureCycle11_rSquared2=310u;
inline constexpr uint32_t pressureCycle11_rSquared3=311u;
inline constexpr uint32_t pressureCycle11_rMaxLo=312u;
inline constexpr uint32_t pressureCycle11_rMaxHi=313u;
inline constexpr uint32_t pressureCycle11_sumBLo=314u;
inline constexpr uint32_t pressureCycle11_sumBHi=315u;
inline constexpr uint32_t pressureCycle11_gaugeMeanLo=316u;
inline constexpr uint32_t pressureCycle11_gaugeMeanHi=317u;
inline constexpr uint32_t pressureCycle11_activeSweeps=318u;
inline constexpr uint32_t pressureCycle11_seal=319u;
inline constexpr uint32_t pressureCycle12_ordinal=320u;
inline constexpr uint32_t pressureCycle12_flags=321u;
inline constexpr uint32_t pressureCycle12_converged=322u;
inline constexpr uint32_t pressureCycle12_fault=323u;
inline constexpr uint32_t pressureCycle12_rSquared0=324u;
inline constexpr uint32_t pressureCycle12_rSquared1=325u;
inline constexpr uint32_t pressureCycle12_rSquared2=326u;
inline constexpr uint32_t pressureCycle12_rSquared3=327u;
inline constexpr uint32_t pressureCycle12_rMaxLo=328u;
inline constexpr uint32_t pressureCycle12_rMaxHi=329u;
inline constexpr uint32_t pressureCycle12_sumBLo=330u;
inline constexpr uint32_t pressureCycle12_sumBHi=331u;
inline constexpr uint32_t pressureCycle12_gaugeMeanLo=332u;
inline constexpr uint32_t pressureCycle12_gaugeMeanHi=333u;
inline constexpr uint32_t pressureCycle12_activeSweeps=334u;
inline constexpr uint32_t pressureCycle12_seal=335u;
inline constexpr uint32_t pressureCycle13_ordinal=336u;
inline constexpr uint32_t pressureCycle13_flags=337u;
inline constexpr uint32_t pressureCycle13_converged=338u;
inline constexpr uint32_t pressureCycle13_fault=339u;
inline constexpr uint32_t pressureCycle13_rSquared0=340u;
inline constexpr uint32_t pressureCycle13_rSquared1=341u;
inline constexpr uint32_t pressureCycle13_rSquared2=342u;
inline constexpr uint32_t pressureCycle13_rSquared3=343u;
inline constexpr uint32_t pressureCycle13_rMaxLo=344u;
inline constexpr uint32_t pressureCycle13_rMaxHi=345u;
inline constexpr uint32_t pressureCycle13_sumBLo=346u;
inline constexpr uint32_t pressureCycle13_sumBHi=347u;
inline constexpr uint32_t pressureCycle13_gaugeMeanLo=348u;
inline constexpr uint32_t pressureCycle13_gaugeMeanHi=349u;
inline constexpr uint32_t pressureCycle13_activeSweeps=350u;
inline constexpr uint32_t pressureCycle13_seal=351u;
inline constexpr uint32_t pressureCycle14_ordinal=352u;
inline constexpr uint32_t pressureCycle14_flags=353u;
inline constexpr uint32_t pressureCycle14_converged=354u;
inline constexpr uint32_t pressureCycle14_fault=355u;
inline constexpr uint32_t pressureCycle14_rSquared0=356u;
inline constexpr uint32_t pressureCycle14_rSquared1=357u;
inline constexpr uint32_t pressureCycle14_rSquared2=358u;
inline constexpr uint32_t pressureCycle14_rSquared3=359u;
inline constexpr uint32_t pressureCycle14_rMaxLo=360u;
inline constexpr uint32_t pressureCycle14_rMaxHi=361u;
inline constexpr uint32_t pressureCycle14_sumBLo=362u;
inline constexpr uint32_t pressureCycle14_sumBHi=363u;
inline constexpr uint32_t pressureCycle14_gaugeMeanLo=364u;
inline constexpr uint32_t pressureCycle14_gaugeMeanHi=365u;
inline constexpr uint32_t pressureCycle14_activeSweeps=366u;
inline constexpr uint32_t pressureCycle14_seal=367u;
inline constexpr uint32_t pressureCycle15_ordinal=368u;
inline constexpr uint32_t pressureCycle15_flags=369u;
inline constexpr uint32_t pressureCycle15_converged=370u;
inline constexpr uint32_t pressureCycle15_fault=371u;
inline constexpr uint32_t pressureCycle15_rSquared0=372u;
inline constexpr uint32_t pressureCycle15_rSquared1=373u;
inline constexpr uint32_t pressureCycle15_rSquared2=374u;
inline constexpr uint32_t pressureCycle15_rSquared3=375u;
inline constexpr uint32_t pressureCycle15_rMaxLo=376u;
inline constexpr uint32_t pressureCycle15_rMaxHi=377u;
inline constexpr uint32_t pressureCycle15_sumBLo=378u;
inline constexpr uint32_t pressureCycle15_sumBHi=379u;
inline constexpr uint32_t pressureCycle15_gaugeMeanLo=380u;
inline constexpr uint32_t pressureCycle15_gaugeMeanHi=381u;
inline constexpr uint32_t pressureCycle15_activeSweeps=382u;
inline constexpr uint32_t pressureCycle15_seal=383u;
inline constexpr uint32_t viscBlock0_ordinal=384u;
inline constexpr uint32_t viscBlock0_flags=385u;
inline constexpr uint32_t viscBlock0_converged=386u;
inline constexpr uint32_t viscBlock0_fault=387u;
inline constexpr uint32_t viscBlock0_trueRMSF32=388u;
inline constexpr uint32_t viscBlock0_trueMaxF32=389u;
inline constexpr uint32_t viscBlock0_lastIteration=390u;
inline constexpr uint32_t viscBlock0_lambdaMaxF32=391u;
inline constexpr uint32_t viscBlock0_rhoF32=392u;
inline constexpr uint32_t viscBlock0_alphaF32=393u;
inline constexpr uint32_t viscBlock0_betaF32=394u;
inline constexpr uint32_t viscBlock0_nuF32=395u;
inline constexpr uint32_t viscBlock0_energyResidualF32=396u;
inline constexpr uint32_t viscBlock0_reserved0=397u;
inline constexpr uint32_t viscBlock0_activeIterations=398u;
inline constexpr uint32_t viscBlock0_seal=399u;
inline constexpr uint32_t viscBlock1_ordinal=400u;
inline constexpr uint32_t viscBlock1_flags=401u;
inline constexpr uint32_t viscBlock1_converged=402u;
inline constexpr uint32_t viscBlock1_fault=403u;
inline constexpr uint32_t viscBlock1_trueRMSF32=404u;
inline constexpr uint32_t viscBlock1_trueMaxF32=405u;
inline constexpr uint32_t viscBlock1_lastIteration=406u;
inline constexpr uint32_t viscBlock1_lambdaMaxF32=407u;
inline constexpr uint32_t viscBlock1_rhoF32=408u;
inline constexpr uint32_t viscBlock1_alphaF32=409u;
inline constexpr uint32_t viscBlock1_betaF32=410u;
inline constexpr uint32_t viscBlock1_nuF32=411u;
inline constexpr uint32_t viscBlock1_energyResidualF32=412u;
inline constexpr uint32_t viscBlock1_reserved0=413u;
inline constexpr uint32_t viscBlock1_activeIterations=414u;
inline constexpr uint32_t viscBlock1_seal=415u;
inline constexpr uint32_t viscBlock2_ordinal=416u;
inline constexpr uint32_t viscBlock2_flags=417u;
inline constexpr uint32_t viscBlock2_converged=418u;
inline constexpr uint32_t viscBlock2_fault=419u;
inline constexpr uint32_t viscBlock2_trueRMSF32=420u;
inline constexpr uint32_t viscBlock2_trueMaxF32=421u;
inline constexpr uint32_t viscBlock2_lastIteration=422u;
inline constexpr uint32_t viscBlock2_lambdaMaxF32=423u;
inline constexpr uint32_t viscBlock2_rhoF32=424u;
inline constexpr uint32_t viscBlock2_alphaF32=425u;
inline constexpr uint32_t viscBlock2_betaF32=426u;
inline constexpr uint32_t viscBlock2_nuF32=427u;
inline constexpr uint32_t viscBlock2_energyResidualF32=428u;
inline constexpr uint32_t viscBlock2_reserved0=429u;
inline constexpr uint32_t viscBlock2_activeIterations=430u;
inline constexpr uint32_t viscBlock2_seal=431u;
inline constexpr uint32_t viscBlock3_ordinal=432u;
inline constexpr uint32_t viscBlock3_flags=433u;
inline constexpr uint32_t viscBlock3_converged=434u;
inline constexpr uint32_t viscBlock3_fault=435u;
inline constexpr uint32_t viscBlock3_trueRMSF32=436u;
inline constexpr uint32_t viscBlock3_trueMaxF32=437u;
inline constexpr uint32_t viscBlock3_lastIteration=438u;
inline constexpr uint32_t viscBlock3_lambdaMaxF32=439u;
inline constexpr uint32_t viscBlock3_rhoF32=440u;
inline constexpr uint32_t viscBlock3_alphaF32=441u;
inline constexpr uint32_t viscBlock3_betaF32=442u;
inline constexpr uint32_t viscBlock3_nuF32=443u;
inline constexpr uint32_t viscBlock3_energyResidualF32=444u;
inline constexpr uint32_t viscBlock3_reserved0=445u;
inline constexpr uint32_t viscBlock3_activeIterations=446u;
inline constexpr uint32_t viscBlock3_seal=447u;
inline constexpr uint32_t viscBlock4_ordinal=448u;
inline constexpr uint32_t viscBlock4_flags=449u;
inline constexpr uint32_t viscBlock4_converged=450u;
inline constexpr uint32_t viscBlock4_fault=451u;
inline constexpr uint32_t viscBlock4_trueRMSF32=452u;
inline constexpr uint32_t viscBlock4_trueMaxF32=453u;
inline constexpr uint32_t viscBlock4_lastIteration=454u;
inline constexpr uint32_t viscBlock4_lambdaMaxF32=455u;
inline constexpr uint32_t viscBlock4_rhoF32=456u;
inline constexpr uint32_t viscBlock4_alphaF32=457u;
inline constexpr uint32_t viscBlock4_betaF32=458u;
inline constexpr uint32_t viscBlock4_nuF32=459u;
inline constexpr uint32_t viscBlock4_energyResidualF32=460u;
inline constexpr uint32_t viscBlock4_reserved0=461u;
inline constexpr uint32_t viscBlock4_activeIterations=462u;
inline constexpr uint32_t viscBlock4_seal=463u;
inline constexpr uint32_t viscBlock5_ordinal=464u;
inline constexpr uint32_t viscBlock5_flags=465u;
inline constexpr uint32_t viscBlock5_converged=466u;
inline constexpr uint32_t viscBlock5_fault=467u;
inline constexpr uint32_t viscBlock5_trueRMSF32=468u;
inline constexpr uint32_t viscBlock5_trueMaxF32=469u;
inline constexpr uint32_t viscBlock5_lastIteration=470u;
inline constexpr uint32_t viscBlock5_lambdaMaxF32=471u;
inline constexpr uint32_t viscBlock5_rhoF32=472u;
inline constexpr uint32_t viscBlock5_alphaF32=473u;
inline constexpr uint32_t viscBlock5_betaF32=474u;
inline constexpr uint32_t viscBlock5_nuF32=475u;
inline constexpr uint32_t viscBlock5_energyResidualF32=476u;
inline constexpr uint32_t viscBlock5_reserved0=477u;
inline constexpr uint32_t viscBlock5_activeIterations=478u;
inline constexpr uint32_t viscBlock5_seal=479u;
inline constexpr uint32_t viscBlock6_ordinal=480u;
inline constexpr uint32_t viscBlock6_flags=481u;
inline constexpr uint32_t viscBlock6_converged=482u;
inline constexpr uint32_t viscBlock6_fault=483u;
inline constexpr uint32_t viscBlock6_trueRMSF32=484u;
inline constexpr uint32_t viscBlock6_trueMaxF32=485u;
inline constexpr uint32_t viscBlock6_lastIteration=486u;
inline constexpr uint32_t viscBlock6_lambdaMaxF32=487u;
inline constexpr uint32_t viscBlock6_rhoF32=488u;
inline constexpr uint32_t viscBlock6_alphaF32=489u;
inline constexpr uint32_t viscBlock6_betaF32=490u;
inline constexpr uint32_t viscBlock6_nuF32=491u;
inline constexpr uint32_t viscBlock6_energyResidualF32=492u;
inline constexpr uint32_t viscBlock6_reserved0=493u;
inline constexpr uint32_t viscBlock6_activeIterations=494u;
inline constexpr uint32_t viscBlock6_seal=495u;
inline constexpr uint32_t viscBlock7_ordinal=496u;
inline constexpr uint32_t viscBlock7_flags=497u;
inline constexpr uint32_t viscBlock7_converged=498u;
inline constexpr uint32_t viscBlock7_fault=499u;
inline constexpr uint32_t viscBlock7_trueRMSF32=500u;
inline constexpr uint32_t viscBlock7_trueMaxF32=501u;
inline constexpr uint32_t viscBlock7_lastIteration=502u;
inline constexpr uint32_t viscBlock7_lambdaMaxF32=503u;
inline constexpr uint32_t viscBlock7_rhoF32=504u;
inline constexpr uint32_t viscBlock7_alphaF32=505u;
inline constexpr uint32_t viscBlock7_betaF32=506u;
inline constexpr uint32_t viscBlock7_nuF32=507u;
inline constexpr uint32_t viscBlock7_energyResidualF32=508u;
inline constexpr uint32_t viscBlock7_reserved0=509u;
inline constexpr uint32_t viscBlock7_activeIterations=510u;
inline constexpr uint32_t viscBlock7_seal=511u;
inline constexpr uint32_t stage0_opcode=512u;
inline constexpr uint32_t stage0_flags=513u;
inline constexpr uint32_t stage0_firstError=514u;
inline constexpr uint32_t stage0_activeElements=515u;
inline constexpr uint32_t stage0_CFLmaxF32=516u;
inline constexpr uint32_t stage0_valueMaxF32=517u;
inline constexpr uint32_t stage0_scalarF32=518u;
inline constexpr uint32_t stage0_seal=519u;
inline constexpr uint32_t stage1_opcode=520u;
inline constexpr uint32_t stage1_flags=521u;
inline constexpr uint32_t stage1_firstError=522u;
inline constexpr uint32_t stage1_activeElements=523u;
inline constexpr uint32_t stage1_CFLmaxF32=524u;
inline constexpr uint32_t stage1_valueMaxF32=525u;
inline constexpr uint32_t stage1_scalarF32=526u;
inline constexpr uint32_t stage1_seal=527u;
inline constexpr uint32_t stage2_opcode=528u;
inline constexpr uint32_t stage2_flags=529u;
inline constexpr uint32_t stage2_firstError=530u;
inline constexpr uint32_t stage2_activeElements=531u;
inline constexpr uint32_t stage2_CFLmaxF32=532u;
inline constexpr uint32_t stage2_valueMaxF32=533u;
inline constexpr uint32_t stage2_scalarF32=534u;
inline constexpr uint32_t stage2_seal=535u;
inline constexpr uint32_t stage3_opcode=536u;
inline constexpr uint32_t stage3_flags=537u;
inline constexpr uint32_t stage3_firstError=538u;
inline constexpr uint32_t stage3_activeElements=539u;
inline constexpr uint32_t stage3_CFLmaxF32=540u;
inline constexpr uint32_t stage3_valueMaxF32=541u;
inline constexpr uint32_t stage3_scalarF32=542u;
inline constexpr uint32_t stage3_seal=543u;
inline constexpr uint32_t stage4_opcode=544u;
inline constexpr uint32_t stage4_flags=545u;
inline constexpr uint32_t stage4_firstError=546u;
inline constexpr uint32_t stage4_activeElements=547u;
inline constexpr uint32_t stage4_CFLmaxF32=548u;
inline constexpr uint32_t stage4_valueMaxF32=549u;
inline constexpr uint32_t stage4_scalarF32=550u;
inline constexpr uint32_t stage4_seal=551u;
inline constexpr uint32_t stage5_opcode=552u;
inline constexpr uint32_t stage5_flags=553u;
inline constexpr uint32_t stage5_firstError=554u;
inline constexpr uint32_t stage5_activeElements=555u;
inline constexpr uint32_t stage5_CFLmaxF32=556u;
inline constexpr uint32_t stage5_valueMaxF32=557u;
inline constexpr uint32_t stage5_scalarF32=558u;
inline constexpr uint32_t stage5_seal=559u;
inline constexpr uint32_t stage6_opcode=560u;
inline constexpr uint32_t stage6_flags=561u;
inline constexpr uint32_t stage6_firstError=562u;
inline constexpr uint32_t stage6_activeElements=563u;
inline constexpr uint32_t stage6_CFLmaxF32=564u;
inline constexpr uint32_t stage6_valueMaxF32=565u;
inline constexpr uint32_t stage6_scalarF32=566u;
inline constexpr uint32_t stage6_seal=567u;
inline constexpr uint32_t stage7_opcode=568u;
inline constexpr uint32_t stage7_flags=569u;
inline constexpr uint32_t stage7_firstError=570u;
inline constexpr uint32_t stage7_activeElements=571u;
inline constexpr uint32_t stage7_CFLmaxF32=572u;
inline constexpr uint32_t stage7_valueMaxF32=573u;
inline constexpr uint32_t stage7_scalarF32=574u;
inline constexpr uint32_t stage7_seal=575u;
inline constexpr uint32_t stage8_opcode=576u;
inline constexpr uint32_t stage8_flags=577u;
inline constexpr uint32_t stage8_firstError=578u;
inline constexpr uint32_t stage8_activeElements=579u;
inline constexpr uint32_t stage8_CFLmaxF32=580u;
inline constexpr uint32_t stage8_valueMaxF32=581u;
inline constexpr uint32_t stage8_scalarF32=582u;
inline constexpr uint32_t stage8_seal=583u;
inline constexpr uint32_t stage9_opcode=584u;
inline constexpr uint32_t stage9_flags=585u;
inline constexpr uint32_t stage9_firstError=586u;
inline constexpr uint32_t stage9_activeElements=587u;
inline constexpr uint32_t stage9_CFLmaxF32=588u;
inline constexpr uint32_t stage9_valueMaxF32=589u;
inline constexpr uint32_t stage9_scalarF32=590u;
inline constexpr uint32_t stage9_seal=591u;
inline constexpr uint32_t stage10_opcode=592u;
inline constexpr uint32_t stage10_flags=593u;
inline constexpr uint32_t stage10_firstError=594u;
inline constexpr uint32_t stage10_activeElements=595u;
inline constexpr uint32_t stage10_CFLmaxF32=596u;
inline constexpr uint32_t stage10_valueMaxF32=597u;
inline constexpr uint32_t stage10_scalarF32=598u;
inline constexpr uint32_t stage10_seal=599u;
inline constexpr uint32_t stage11_opcode=600u;
inline constexpr uint32_t stage11_flags=601u;
inline constexpr uint32_t stage11_firstError=602u;
inline constexpr uint32_t stage11_activeElements=603u;
inline constexpr uint32_t stage11_CFLmaxF32=604u;
inline constexpr uint32_t stage11_valueMaxF32=605u;
inline constexpr uint32_t stage11_scalarF32=606u;
inline constexpr uint32_t stage11_seal=607u;
inline constexpr uint32_t stage12_opcode=608u;
inline constexpr uint32_t stage12_flags=609u;
inline constexpr uint32_t stage12_firstError=610u;
inline constexpr uint32_t stage12_activeElements=611u;
inline constexpr uint32_t stage12_CFLmaxF32=612u;
inline constexpr uint32_t stage12_valueMaxF32=613u;
inline constexpr uint32_t stage12_scalarF32=614u;
inline constexpr uint32_t stage12_seal=615u;
inline constexpr uint32_t stage13_opcode=616u;
inline constexpr uint32_t stage13_flags=617u;
inline constexpr uint32_t stage13_firstError=618u;
inline constexpr uint32_t stage13_activeElements=619u;
inline constexpr uint32_t stage13_CFLmaxF32=620u;
inline constexpr uint32_t stage13_valueMaxF32=621u;
inline constexpr uint32_t stage13_scalarF32=622u;
inline constexpr uint32_t stage13_seal=623u;
inline constexpr uint32_t stage14_opcode=624u;
inline constexpr uint32_t stage14_flags=625u;
inline constexpr uint32_t stage14_firstError=626u;
inline constexpr uint32_t stage14_activeElements=627u;
inline constexpr uint32_t stage14_CFLmaxF32=628u;
inline constexpr uint32_t stage14_valueMaxF32=629u;
inline constexpr uint32_t stage14_scalarF32=630u;
inline constexpr uint32_t stage14_seal=631u;
inline constexpr uint32_t stage15_opcode=632u;
inline constexpr uint32_t stage15_flags=633u;
inline constexpr uint32_t stage15_firstError=634u;
inline constexpr uint32_t stage15_activeElements=635u;
inline constexpr uint32_t stage15_CFLmaxF32=636u;
inline constexpr uint32_t stage15_valueMaxF32=637u;
inline constexpr uint32_t stage15_scalarF32=638u;
inline constexpr uint32_t stage15_seal=639u;
inline constexpr uint32_t stage16_opcode=640u;
inline constexpr uint32_t stage16_flags=641u;
inline constexpr uint32_t stage16_firstError=642u;
inline constexpr uint32_t stage16_activeElements=643u;
inline constexpr uint32_t stage16_CFLmaxF32=644u;
inline constexpr uint32_t stage16_valueMaxF32=645u;
inline constexpr uint32_t stage16_scalarF32=646u;
inline constexpr uint32_t stage16_seal=647u;
inline constexpr uint32_t stage17_opcode=648u;
inline constexpr uint32_t stage17_flags=649u;
inline constexpr uint32_t stage17_firstError=650u;
inline constexpr uint32_t stage17_activeElements=651u;
inline constexpr uint32_t stage17_CFLmaxF32=652u;
inline constexpr uint32_t stage17_valueMaxF32=653u;
inline constexpr uint32_t stage17_scalarF32=654u;
inline constexpr uint32_t stage17_seal=655u;
inline constexpr uint32_t stage18_opcode=656u;
inline constexpr uint32_t stage18_flags=657u;
inline constexpr uint32_t stage18_firstError=658u;
inline constexpr uint32_t stage18_activeElements=659u;
inline constexpr uint32_t stage18_CFLmaxF32=660u;
inline constexpr uint32_t stage18_valueMaxF32=661u;
inline constexpr uint32_t stage18_scalarF32=662u;
inline constexpr uint32_t stage18_seal=663u;
inline constexpr uint32_t stage19_opcode=664u;
inline constexpr uint32_t stage19_flags=665u;
inline constexpr uint32_t stage19_firstError=666u;
inline constexpr uint32_t stage19_activeElements=667u;
inline constexpr uint32_t stage19_CFLmaxF32=668u;
inline constexpr uint32_t stage19_valueMaxF32=669u;
inline constexpr uint32_t stage19_scalarF32=670u;
inline constexpr uint32_t stage19_seal=671u;
inline constexpr uint32_t stage20_opcode=672u;
inline constexpr uint32_t stage20_flags=673u;
inline constexpr uint32_t stage20_firstError=674u;
inline constexpr uint32_t stage20_activeElements=675u;
inline constexpr uint32_t stage20_CFLmaxF32=676u;
inline constexpr uint32_t stage20_valueMaxF32=677u;
inline constexpr uint32_t stage20_scalarF32=678u;
inline constexpr uint32_t stage20_seal=679u;
inline constexpr uint32_t stage21_opcode=680u;
inline constexpr uint32_t stage21_flags=681u;
inline constexpr uint32_t stage21_firstError=682u;
inline constexpr uint32_t stage21_activeElements=683u;
inline constexpr uint32_t stage21_CFLmaxF32=684u;
inline constexpr uint32_t stage21_valueMaxF32=685u;
inline constexpr uint32_t stage21_scalarF32=686u;
inline constexpr uint32_t stage21_seal=687u;
inline constexpr uint32_t stage22_opcode=688u;
inline constexpr uint32_t stage22_flags=689u;
inline constexpr uint32_t stage22_firstError=690u;
inline constexpr uint32_t stage22_activeElements=691u;
inline constexpr uint32_t stage22_CFLmaxF32=692u;
inline constexpr uint32_t stage22_valueMaxF32=693u;
inline constexpr uint32_t stage22_scalarF32=694u;
inline constexpr uint32_t stage22_seal=695u;
inline constexpr uint32_t stage23_opcode=696u;
inline constexpr uint32_t stage23_flags=697u;
inline constexpr uint32_t stage23_firstError=698u;
inline constexpr uint32_t stage23_activeElements=699u;
inline constexpr uint32_t stage23_CFLmaxF32=700u;
inline constexpr uint32_t stage23_valueMaxF32=701u;
inline constexpr uint32_t stage23_scalarF32=702u;
inline constexpr uint32_t stage23_seal=703u;
inline constexpr uint32_t stage24_opcode=704u;
inline constexpr uint32_t stage24_flags=705u;
inline constexpr uint32_t stage24_firstError=706u;
inline constexpr uint32_t stage24_activeElements=707u;
inline constexpr uint32_t stage24_CFLmaxF32=708u;
inline constexpr uint32_t stage24_valueMaxF32=709u;
inline constexpr uint32_t stage24_scalarF32=710u;
inline constexpr uint32_t stage24_seal=711u;
inline constexpr uint32_t stage25_opcode=712u;
inline constexpr uint32_t stage25_flags=713u;
inline constexpr uint32_t stage25_firstError=714u;
inline constexpr uint32_t stage25_activeElements=715u;
inline constexpr uint32_t stage25_CFLmaxF32=716u;
inline constexpr uint32_t stage25_valueMaxF32=717u;
inline constexpr uint32_t stage25_scalarF32=718u;
inline constexpr uint32_t stage25_seal=719u;
inline constexpr uint32_t stage26_opcode=720u;
inline constexpr uint32_t stage26_flags=721u;
inline constexpr uint32_t stage26_firstError=722u;
inline constexpr uint32_t stage26_activeElements=723u;
inline constexpr uint32_t stage26_CFLmaxF32=724u;
inline constexpr uint32_t stage26_valueMaxF32=725u;
inline constexpr uint32_t stage26_scalarF32=726u;
inline constexpr uint32_t stage26_seal=727u;
inline constexpr uint32_t stage27_opcode=728u;
inline constexpr uint32_t stage27_flags=729u;
inline constexpr uint32_t stage27_firstError=730u;
inline constexpr uint32_t stage27_activeElements=731u;
inline constexpr uint32_t stage27_CFLmaxF32=732u;
inline constexpr uint32_t stage27_valueMaxF32=733u;
inline constexpr uint32_t stage27_scalarF32=734u;
inline constexpr uint32_t stage27_seal=735u;
inline constexpr uint32_t stage28_opcode=736u;
inline constexpr uint32_t stage28_flags=737u;
inline constexpr uint32_t stage28_firstError=738u;
inline constexpr uint32_t stage28_activeElements=739u;
inline constexpr uint32_t stage28_CFLmaxF32=740u;
inline constexpr uint32_t stage28_valueMaxF32=741u;
inline constexpr uint32_t stage28_scalarF32=742u;
inline constexpr uint32_t stage28_seal=743u;
inline constexpr uint32_t stage29_opcode=744u;
inline constexpr uint32_t stage29_flags=745u;
inline constexpr uint32_t stage29_firstError=746u;
inline constexpr uint32_t stage29_activeElements=747u;
inline constexpr uint32_t stage29_CFLmaxF32=748u;
inline constexpr uint32_t stage29_valueMaxF32=749u;
inline constexpr uint32_t stage29_scalarF32=750u;
inline constexpr uint32_t stage29_seal=751u;
inline constexpr uint32_t stage30_opcode=752u;
inline constexpr uint32_t stage30_flags=753u;
inline constexpr uint32_t stage30_firstError=754u;
inline constexpr uint32_t stage30_activeElements=755u;
inline constexpr uint32_t stage30_CFLmaxF32=756u;
inline constexpr uint32_t stage30_valueMaxF32=757u;
inline constexpr uint32_t stage30_scalarF32=758u;
inline constexpr uint32_t stage30_seal=759u;
inline constexpr uint32_t stage31_opcode=760u;
inline constexpr uint32_t stage31_flags=761u;
inline constexpr uint32_t stage31_firstError=762u;
inline constexpr uint32_t stage31_activeElements=763u;
inline constexpr uint32_t stage31_CFLmaxF32=764u;
inline constexpr uint32_t stage31_valueMaxF32=765u;
inline constexpr uint32_t stage31_scalarF32=766u;
inline constexpr uint32_t stage31_seal=767u;
inline constexpr uint32_t thetaWrappedF32=768u;
inline constexpr uint32_t OmegaF32_769=769u;
inline constexpr uint32_t alphaF32_770=770u;
inline constexpr uint32_t gxF32_771=771u;
inline constexpr uint32_t gyF32_772=772u;
inline constexpr uint32_t phiMeanF32=773u;
inline constexpr uint32_t pressureReferenceF32=774u;
inline constexpr uint32_t forceAmpF32=775u;
inline constexpr uint32_t deltaF32=776u;
inline constexpr uint32_t nuF32_777=777u;
inline constexpr uint32_t hF32_778=778u;
inline constexpr uint32_t wallBottomF32=779u;
inline constexpr uint32_t wallTopF32=780u;
inline constexpr uint32_t positiveWorkCapF32=781u;
inline constexpr uint32_t forceOn=782u;
inline constexpr uint32_t mathStatus=783u;
inline constexpr uint32_t theta_binary128_0=784u;
inline constexpr uint32_t theta_binary128_1=785u;
inline constexpr uint32_t theta_binary128_2=786u;
inline constexpr uint32_t theta_binary128_3=787u;
inline constexpr uint32_t Omega_binary128_0=788u;
inline constexpr uint32_t Omega_binary128_1=789u;
inline constexpr uint32_t Omega_binary128_2=790u;
inline constexpr uint32_t Omega_binary128_3=791u;
inline constexpr uint32_t alpha_binary128_0=792u;
inline constexpr uint32_t alpha_binary128_1=793u;
inline constexpr uint32_t alpha_binary128_2=794u;
inline constexpr uint32_t alpha_binary128_3=795u;
inline constexpr uint32_t gx_binary128_0=796u;
inline constexpr uint32_t gx_binary128_1=797u;
inline constexpr uint32_t gx_binary128_2=798u;
inline constexpr uint32_t gx_binary128_3=799u;
inline constexpr uint32_t gy_binary128_0=800u;
inline constexpr uint32_t gy_binary128_1=801u;
inline constexpr uint32_t gy_binary128_2=802u;
inline constexpr uint32_t gy_binary128_3=803u;
inline constexpr uint32_t tickBefore_lo=804u;
inline constexpr uint32_t tickBefore_hi=805u;
inline constexpr uint32_t tickAfter_lo=806u;
inline constexpr uint32_t tickAfter_hi=807u;
inline constexpr uint32_t mathSHA_0=808u;
inline constexpr uint32_t mathSHA_1=809u;
inline constexpr uint32_t mathSHA_2=810u;
inline constexpr uint32_t mathSHA_3=811u;
inline constexpr uint32_t mathSHA_4=812u;
inline constexpr uint32_t mathSHA_5=813u;
inline constexpr uint32_t mathSHA_6=814u;
inline constexpr uint32_t mathSHA_7=815u;
inline constexpr uint32_t reserved0_48=816u;
inline constexpr uint32_t reserved0_49=817u;
inline constexpr uint32_t reserved0_50=818u;
inline constexpr uint32_t reserved0_51=819u;
inline constexpr uint32_t reserved0_52_820=820u;
inline constexpr uint32_t reserved0_53_821=821u;
inline constexpr uint32_t reserved0_54_822=822u;
inline constexpr uint32_t reserved0_55_823=823u;
inline constexpr uint32_t reserved0_56_824=824u;
inline constexpr uint32_t reserved0_57_825=825u;
inline constexpr uint32_t reserved0_58_826=826u;
inline constexpr uint32_t reserved0_59_827=827u;
inline constexpr uint32_t reserved0_60_828=828u;
inline constexpr uint32_t reserved0_61_829=829u;
inline constexpr uint32_t reserved0_62_830=830u;
inline constexpr uint32_t reserved0_63_831=831u;
inline constexpr uint32_t KwindowStart_valueF32=832u;
inline constexpr uint32_t KwindowStart_errorBoundF32=833u;
inline constexpr uint32_t K_valueF32=834u;
inline constexpr uint32_t K_errorBoundF32=835u;
inline constexpr uint32_t WEuler_valueF32=836u;
inline constexpr uint32_t WEuler_errorBoundF32=837u;
inline constexpr uint32_t Wnonpotential_valueF32=838u;
inline constexpr uint32_t Wnonpotential_errorBoundF32=839u;
inline constexpr uint32_t Wwall_valueF32=840u;
inline constexpr uint32_t Wwall_errorBoundF32=841u;
inline constexpr uint32_t Wpositive_valueF32=842u;
inline constexpr uint32_t Wpositive_errorBoundF32=843u;
inline constexpr uint32_t Dadv_valueF32=844u;
inline constexpr uint32_t Dadv_errorBoundF32=845u;
inline constexpr uint32_t Dnu_valueF32=846u;
inline constexpr uint32_t Dnu_errorBoundF32=847u;
inline constexpr uint32_t DBE_valueF32=848u;
inline constexpr uint32_t DBE_errorBoundF32=849u;
inline constexpr uint32_t Dprojection_valueF32=850u;
inline constexpr uint32_t Dprojection_errorBoundF32=851u;
inline constexpr uint32_t Rdualdiv_valueF32=852u;
inline constexpr uint32_t Rdualdiv_errorBoundF32=853u;
inline constexpr uint32_t RH_valueF32=854u;
inline constexpr uint32_t RH_errorBoundF32=855u;
inline constexpr uint32_t RP_valueF32=856u;
inline constexpr uint32_t RP_errorBoundF32=857u;
inline constexpr uint32_t RrhsQuantization_valueF32=858u;
inline constexpr uint32_t RrhsQuantization_errorBoundF32=859u;
inline constexpr uint32_t Rfloat_valueF32=860u;
inline constexpr uint32_t Rfloat_errorBoundF32=861u;
inline constexpr uint32_t windowAbsWork_valueF32=862u;
inline constexpr uint32_t windowAbsWork_errorBoundF32=863u;
inline constexpr uint32_t windowDissipation_valueF32=864u;
inline constexpr uint32_t windowDissipation_errorBoundF32=865u;
inline constexpr uint32_t Escalefrozen_valueF32=866u;
inline constexpr uint32_t Escalefrozen_errorBoundF32=867u;
inline constexpr uint32_t momentumClosureX_valueF32=868u;
inline constexpr uint32_t momentumClosureX_errorBoundF32=869u;
inline constexpr uint32_t momentumClosureY_valueF32=870u;
inline constexpr uint32_t momentumClosureY_errorBoundF32=871u;
inline constexpr uint32_t CFLincoming_valueF32=872u;
inline constexpr uint32_t CFLincoming_errorBoundF32=873u;
inline constexpr uint32_t CFLoutgoing_valueF32=874u;
inline constexpr uint32_t CFLoutgoing_errorBoundF32=875u;
inline constexpr uint32_t divergenceRMS_valueF32=876u;
inline constexpr uint32_t divergenceRMS_errorBoundF32=877u;
inline constexpr uint32_t divergenceMax_valueF32=878u;
inline constexpr uint32_t divergenceMax_errorBoundF32=879u;
inline constexpr uint32_t windowAcceptedQuanta=880u;
inline constexpr uint32_t flags=881u;
inline constexpr uint32_t windowFirstQuantum_lo=882u;
inline constexpr uint32_t windowFirstQuantum_hi=883u;
inline constexpr uint32_t windowLastQuantum_lo=884u;
inline constexpr uint32_t windowLastQuantum_hi=885u;
inline constexpr uint32_t externalErrorBoundF32=886u;
inline constexpr uint32_t priorEscalefrozenF32=887u;
inline constexpr uint32_t preKickKF32=888u;
inline constexpr uint32_t postKickKF32=889u;
inline constexpr uint32_t floatBudgetF32=890u;
inline constexpr uint32_t ledgerFlags=891u;
inline constexpr uint32_t reserved0_60_892=892u;
inline constexpr uint32_t reserved0_61_893=893u;
inline constexpr uint32_t reserved0_62_894=894u;
inline constexpr uint32_t reserved0_63_895=895u;
inline constexpr uint32_t wall0_pressureJxF32=896u;
inline constexpr uint32_t wall0_pressureJyF32=897u;
inline constexpr uint32_t wall0_viscJxF32=898u;
inline constexpr uint32_t wall0_viscJyF32=899u;
inline constexpr uint32_t wall0_workF32=900u;
inline constexpr uint32_t wall0_meanPressureF32=901u;
inline constexpr uint32_t wall0_flags=902u;
inline constexpr uint32_t wall0_reserved0=903u;
inline constexpr uint32_t wall1_pressureJxF32=904u;
inline constexpr uint32_t wall1_pressureJyF32=905u;
inline constexpr uint32_t wall1_viscJxF32=906u;
inline constexpr uint32_t wall1_viscJyF32=907u;
inline constexpr uint32_t wall1_workF32=908u;
inline constexpr uint32_t wall1_meanPressureF32=909u;
inline constexpr uint32_t wall1_flags=910u;
inline constexpr uint32_t wall1_reserved0=911u;
inline constexpr uint32_t wall2_pressureJxF32=912u;
inline constexpr uint32_t wall2_pressureJyF32=913u;
inline constexpr uint32_t wall2_viscJxF32=914u;
inline constexpr uint32_t wall2_viscJyF32=915u;
inline constexpr uint32_t wall2_workF32=916u;
inline constexpr uint32_t wall2_meanPressureF32=917u;
inline constexpr uint32_t wall2_flags=918u;
inline constexpr uint32_t wall2_reserved0=919u;
inline constexpr uint32_t wall3_pressureJxF32=920u;
inline constexpr uint32_t wall3_pressureJyF32=921u;
inline constexpr uint32_t wall3_viscJxF32=922u;
inline constexpr uint32_t wall3_viscJyF32=923u;
inline constexpr uint32_t wall3_workF32=924u;
inline constexpr uint32_t wall3_meanPressureF32=925u;
inline constexpr uint32_t wall3_flags=926u;
inline constexpr uint32_t wall3_reserved0=927u;
inline constexpr uint32_t body_xF32=928u;
inline constexpr uint32_t body_yF32=929u;
inline constexpr uint32_t coriolis_xF32=930u;
inline constexpr uint32_t coriolis_yF32=931u;
inline constexpr uint32_t viscous_xF32=932u;
inline constexpr uint32_t viscous_yF32=933u;
inline constexpr uint32_t pressure_xF32=934u;
inline constexpr uint32_t pressure_yF32=935u;
inline constexpr uint32_t normalConstraint_xF32=936u;
inline constexpr uint32_t normalConstraint_yF32=937u;
inline constexpr uint32_t advection_xF32=938u;
inline constexpr uint32_t advection_yF32=939u;
inline constexpr uint32_t phiTraction_xF32=940u;
inline constexpr uint32_t phiTraction_yF32=941u;
inline constexpr uint32_t deltaP_xF32=942u;
inline constexpr uint32_t deltaP_yF32=943u;
inline constexpr uint32_t closure_xF32=944u;
inline constexpr uint32_t closure_yF32=945u;
inline constexpr uint32_t sumAbsJ_xF32=946u;
inline constexpr uint32_t sumAbsJ_yF32=947u;
inline constexpr uint32_t reserved0_52_948=948u;
inline constexpr uint32_t reserved0_53_949=949u;
inline constexpr uint32_t reserved0_54_950=950u;
inline constexpr uint32_t reserved0_55_951=951u;
inline constexpr uint32_t reserved0_56_952=952u;
inline constexpr uint32_t reserved0_57_953=953u;
inline constexpr uint32_t reserved0_58_954=954u;
inline constexpr uint32_t reserved0_59_955=955u;
inline constexpr uint32_t reserved0_60_956=956u;
inline constexpr uint32_t reserved0_61_957=957u;
inline constexpr uint32_t reserved0_62_958=958u;
inline constexpr uint32_t reserved0_63_959=959u;
inline constexpr uint32_t sumBBeforeQ32_lo=960u;
inline constexpr uint32_t sumBBeforeQ32_hi=961u;
inline constexpr uint32_t sumBAfterQ32_lo=962u;
inline constexpr uint32_t sumBAfterQ32_hi=963u;
inline constexpr uint32_t maxCorrectionLSB=964u;
inline constexpr uint32_t correctionCount=965u;
inline constexpr uint32_t piMeanSumQ32_lo=966u;
inline constexpr uint32_t piMeanSumQ32_hi=967u;
inline constexpr uint32_t rPSquared128_0=968u;
inline constexpr uint32_t rPSquared128_1=969u;
inline constexpr uint32_t rPSquared128_2=970u;
inline constexpr uint32_t rPSquared128_3=971u;
inline constexpr uint32_t rPMaxQ32_lo=972u;
inline constexpr uint32_t rPMaxQ32_hi=973u;
inline constexpr uint32_t etaBF32_974=974u;
inline constexpr uint32_t lambdaLowerBoundF32_975=975u;
inline constexpr uint32_t pressureCertificateF32=976u;
inline constexpr uint32_t velocityCertificateF32=977u;
inline constexpr uint32_t projectionIdentityF32=978u;
inline constexpr uint32_t proofFlags=979u;
inline constexpr uint32_t tightPressureToleranceF32=980u;
inline constexpr uint32_t hInverse=981u;
inline constexpr uint32_t J=982u;
inline constexpr uint32_t roundPolicy=983u;
inline constexpr uint32_t diagnosticReason=984u;
inline constexpr uint32_t actualDivRMSF32=985u;
inline constexpr uint32_t actualDivMaxF32=986u;
inline constexpr uint32_t reserved0_27_987=987u;
inline constexpr uint32_t reserved0_28_988=988u;
inline constexpr uint32_t reserved0_29_989=989u;
inline constexpr uint32_t reserved0_30_990=990u;
inline constexpr uint32_t reserved0_31_991=991u;
inline constexpr uint32_t reserved0_992=992u;
inline constexpr uint32_t reserved0_993=993u;
inline constexpr uint32_t reserved0_994=994u;
inline constexpr uint32_t reserved0_995=995u;
inline constexpr uint32_t reserved0_996=996u;
inline constexpr uint32_t reserved0_997=997u;
inline constexpr uint32_t reserved0_998=998u;
inline constexpr uint32_t reserved0_999=999u;
inline constexpr uint32_t reserved0_1000=1000u;
inline constexpr uint32_t reserved0_1001=1001u;
inline constexpr uint32_t reserved0_1002=1002u;
inline constexpr uint32_t reserved0_1003=1003u;
inline constexpr uint32_t reserved0_1004=1004u;
inline constexpr uint32_t reserved0_1005=1005u;
inline constexpr uint32_t reserved0_1006=1006u;
inline constexpr uint32_t reserved0_1007=1007u;
inline constexpr uint32_t reserved0_1008=1008u;
inline constexpr uint32_t reserved0_1009=1009u;
inline constexpr uint32_t reserved0_1010=1010u;
inline constexpr uint32_t reserved0_1011=1011u;
inline constexpr uint32_t reserved0_1012=1012u;
inline constexpr uint32_t reserved0_1013=1013u;
inline constexpr uint32_t reserved0_1014=1014u;
inline constexpr uint32_t reserved0_1015=1015u;
inline constexpr uint32_t reserved0_1016=1016u;
inline constexpr uint32_t reserved0_1017=1017u;
inline constexpr uint32_t reserved0_1018=1018u;
inline constexpr uint32_t reserved0_1019=1019u;
inline constexpr uint32_t reserved0_1020=1020u;
inline constexpr uint32_t reserved0_1021=1021u;
inline constexpr uint32_t reserved0_1022=1022u;
inline constexpr uint32_t seal=1023u;
}
namespace RootTrace1024 {
inline constexpr uint32_t magic=0u;
inline constexpr uint32_t schema=1u;
inline constexpr uint32_t profile=2u;
inline constexpr uint32_t lengthWords=3u;
inline constexpr uint32_t scope=4u;
inline constexpr uint32_t status=5u;
inline constexpr uint32_t rootKind=6u;
inline constexpr uint32_t flags=7u;
inline constexpr uint32_t nonce_lo=8u;
inline constexpr uint32_t nonce_hi=9u;
inline constexpr uint32_t plan_lo=10u;
inline constexpr uint32_t plan_hi=11u;
inline constexpr uint32_t baseCommit_lo=12u;
inline constexpr uint32_t baseCommit_hi=13u;
inline constexpr uint32_t selectedJ=14u;
inline constexpr uint32_t firstFailure=15u;
inline constexpr uint32_t inputSHA_0=16u;
inline constexpr uint32_t inputSHA_1=17u;
inline constexpr uint32_t inputSHA_2=18u;
inline constexpr uint32_t inputSHA_3=19u;
inline constexpr uint32_t inputSHA_4=20u;
inline constexpr uint32_t inputSHA_5=21u;
inline constexpr uint32_t inputSHA_6=22u;
inline constexpr uint32_t inputSHA_7=23u;
inline constexpr uint32_t configSHA_0=24u;
inline constexpr uint32_t configSHA_1=25u;
inline constexpr uint32_t configSHA_2=26u;
inline constexpr uint32_t configSHA_3=27u;
inline constexpr uint32_t configSHA_4=28u;
inline constexpr uint32_t configSHA_5=29u;
inline constexpr uint32_t configSHA_6=30u;
inline constexpr uint32_t configSHA_7=31u;
inline constexpr uint32_t reserved0_32=32u;
inline constexpr uint32_t reserved0_33=33u;
inline constexpr uint32_t reserved0_34=34u;
inline constexpr uint32_t reserved0_35=35u;
inline constexpr uint32_t reserved0_36=36u;
inline constexpr uint32_t reserved0_37=37u;
inline constexpr uint32_t reserved0_38=38u;
inline constexpr uint32_t reserved0_39=39u;
inline constexpr uint32_t reserved0_40=40u;
inline constexpr uint32_t reserved0_41=41u;
inline constexpr uint32_t reserved0_42=42u;
inline constexpr uint32_t reserved0_43=43u;
inline constexpr uint32_t reserved0_44=44u;
inline constexpr uint32_t reserved0_45=45u;
inline constexpr uint32_t reserved0_46=46u;
inline constexpr uint32_t reserved0_47=47u;
inline constexpr uint32_t reserved0_48=48u;
inline constexpr uint32_t reserved0_49=49u;
inline constexpr uint32_t reserved0_50=50u;
inline constexpr uint32_t reserved0_51=51u;
inline constexpr uint32_t reserved0_52=52u;
inline constexpr uint32_t reserved0_53=53u;
inline constexpr uint32_t reserved0_54=54u;
inline constexpr uint32_t reserved0_55=55u;
inline constexpr uint32_t reserved0_56=56u;
inline constexpr uint32_t reserved0_57=57u;
inline constexpr uint32_t reserved0_58=58u;
inline constexpr uint32_t reserved0_59=59u;
inline constexpr uint32_t reserved0_60=60u;
inline constexpr uint32_t reserved0_61=61u;
inline constexpr uint32_t reserved0_62=62u;
inline constexpr uint32_t reserved0_63=63u;
inline constexpr uint32_t trial1_J=64u;
inline constexpr uint32_t trial1_status=65u;
inline constexpr uint32_t trial1_firstErrorOp=66u;
inline constexpr uint32_t trial1_firstError=67u;
inline constexpr uint32_t trial1_attemptedMicro=68u;
inline constexpr uint32_t trial1_sealedMicro=69u;
inline constexpr uint32_t trial1_selected=70u;
inline constexpr uint32_t trial1_requiredMicro=71u;
inline constexpr uint32_t trial1_activeDispatches=72u;
inline constexpr uint32_t trial1_noopDispatches=73u;
inline constexpr uint32_t trial1_positiveWorkF32=74u;
inline constexpr uint32_t trial1_floatRemainderF32=75u;
inline constexpr uint32_t trial1_baseCommit_lo=76u;
inline constexpr uint32_t trial1_baseCommit_hi=77u;
inline constexpr uint32_t trial1_proposedCommit_lo=78u;
inline constexpr uint32_t trial1_proposedCommit_hi=79u;
inline constexpr uint32_t trial1_inputSHA_0=80u;
inline constexpr uint32_t trial1_inputSHA_1=81u;
inline constexpr uint32_t trial1_inputSHA_2=82u;
inline constexpr uint32_t trial1_inputSHA_3=83u;
inline constexpr uint32_t trial1_inputSHA_4=84u;
inline constexpr uint32_t trial1_inputSHA_5=85u;
inline constexpr uint32_t trial1_inputSHA_6=86u;
inline constexpr uint32_t trial1_inputSHA_7=87u;
inline constexpr uint32_t trial1_fieldSHA_0=88u;
inline constexpr uint32_t trial1_fieldSHA_1=89u;
inline constexpr uint32_t trial1_fieldSHA_2=90u;
inline constexpr uint32_t trial1_fieldSHA_3=91u;
inline constexpr uint32_t trial1_fieldSHA_4=92u;
inline constexpr uint32_t trial1_fieldSHA_5=93u;
inline constexpr uint32_t trial1_fieldSHA_6=94u;
inline constexpr uint32_t trial1_fieldSHA_7=95u;
inline constexpr uint32_t trial1_reserved0_32=96u;
inline constexpr uint32_t trial1_reserved0_33=97u;
inline constexpr uint32_t trial1_reserved0_34=98u;
inline constexpr uint32_t trial1_reserved0_35=99u;
inline constexpr uint32_t trial1_reserved0_36=100u;
inline constexpr uint32_t trial1_reserved0_37=101u;
inline constexpr uint32_t trial1_reserved0_38=102u;
inline constexpr uint32_t trial1_reserved0_39=103u;
inline constexpr uint32_t trial1_reserved0_40=104u;
inline constexpr uint32_t trial1_reserved0_41=105u;
inline constexpr uint32_t trial1_reserved0_42=106u;
inline constexpr uint32_t trial1_reserved0_43=107u;
inline constexpr uint32_t trial1_reserved0_44=108u;
inline constexpr uint32_t trial1_reserved0_45=109u;
inline constexpr uint32_t trial1_reserved0_46=110u;
inline constexpr uint32_t trial1_reserved0_47=111u;
inline constexpr uint32_t trial1_reserved0_48=112u;
inline constexpr uint32_t trial1_reserved0_49=113u;
inline constexpr uint32_t trial1_reserved0_50=114u;
inline constexpr uint32_t trial1_reserved0_51=115u;
inline constexpr uint32_t trial1_reserved0_52=116u;
inline constexpr uint32_t trial1_reserved0_53=117u;
inline constexpr uint32_t trial1_reserved0_54=118u;
inline constexpr uint32_t trial1_reserved0_55=119u;
inline constexpr uint32_t trial1_reserved0_56=120u;
inline constexpr uint32_t trial1_reserved0_57=121u;
inline constexpr uint32_t trial1_reserved0_58=122u;
inline constexpr uint32_t trial1_reserved0_59=123u;
inline constexpr uint32_t trial1_reserved0_60=124u;
inline constexpr uint32_t trial1_reserved0_61=125u;
inline constexpr uint32_t trial1_reserved0_62=126u;
inline constexpr uint32_t trial1_seal=127u;
inline constexpr uint32_t trial2_J=128u;
inline constexpr uint32_t trial2_status=129u;
inline constexpr uint32_t trial2_firstErrorOp=130u;
inline constexpr uint32_t trial2_firstError=131u;
inline constexpr uint32_t trial2_attemptedMicro=132u;
inline constexpr uint32_t trial2_sealedMicro=133u;
inline constexpr uint32_t trial2_selected=134u;
inline constexpr uint32_t trial2_requiredMicro=135u;
inline constexpr uint32_t trial2_activeDispatches=136u;
inline constexpr uint32_t trial2_noopDispatches=137u;
inline constexpr uint32_t trial2_positiveWorkF32=138u;
inline constexpr uint32_t trial2_floatRemainderF32=139u;
inline constexpr uint32_t trial2_baseCommit_lo=140u;
inline constexpr uint32_t trial2_baseCommit_hi=141u;
inline constexpr uint32_t trial2_proposedCommit_lo=142u;
inline constexpr uint32_t trial2_proposedCommit_hi=143u;
inline constexpr uint32_t trial2_inputSHA_0=144u;
inline constexpr uint32_t trial2_inputSHA_1=145u;
inline constexpr uint32_t trial2_inputSHA_2=146u;
inline constexpr uint32_t trial2_inputSHA_3=147u;
inline constexpr uint32_t trial2_inputSHA_4=148u;
inline constexpr uint32_t trial2_inputSHA_5=149u;
inline constexpr uint32_t trial2_inputSHA_6=150u;
inline constexpr uint32_t trial2_inputSHA_7=151u;
inline constexpr uint32_t trial2_fieldSHA_0=152u;
inline constexpr uint32_t trial2_fieldSHA_1=153u;
inline constexpr uint32_t trial2_fieldSHA_2=154u;
inline constexpr uint32_t trial2_fieldSHA_3=155u;
inline constexpr uint32_t trial2_fieldSHA_4=156u;
inline constexpr uint32_t trial2_fieldSHA_5=157u;
inline constexpr uint32_t trial2_fieldSHA_6=158u;
inline constexpr uint32_t trial2_fieldSHA_7=159u;
inline constexpr uint32_t trial2_reserved0_32=160u;
inline constexpr uint32_t trial2_reserved0_33=161u;
inline constexpr uint32_t trial2_reserved0_34=162u;
inline constexpr uint32_t trial2_reserved0_35=163u;
inline constexpr uint32_t trial2_reserved0_36=164u;
inline constexpr uint32_t trial2_reserved0_37=165u;
inline constexpr uint32_t trial2_reserved0_38=166u;
inline constexpr uint32_t trial2_reserved0_39=167u;
inline constexpr uint32_t trial2_reserved0_40=168u;
inline constexpr uint32_t trial2_reserved0_41=169u;
inline constexpr uint32_t trial2_reserved0_42=170u;
inline constexpr uint32_t trial2_reserved0_43=171u;
inline constexpr uint32_t trial2_reserved0_44=172u;
inline constexpr uint32_t trial2_reserved0_45=173u;
inline constexpr uint32_t trial2_reserved0_46=174u;
inline constexpr uint32_t trial2_reserved0_47=175u;
inline constexpr uint32_t trial2_reserved0_48=176u;
inline constexpr uint32_t trial2_reserved0_49=177u;
inline constexpr uint32_t trial2_reserved0_50=178u;
inline constexpr uint32_t trial2_reserved0_51=179u;
inline constexpr uint32_t trial2_reserved0_52=180u;
inline constexpr uint32_t trial2_reserved0_53=181u;
inline constexpr uint32_t trial2_reserved0_54=182u;
inline constexpr uint32_t trial2_reserved0_55=183u;
inline constexpr uint32_t trial2_reserved0_56=184u;
inline constexpr uint32_t trial2_reserved0_57=185u;
inline constexpr uint32_t trial2_reserved0_58=186u;
inline constexpr uint32_t trial2_reserved0_59=187u;
inline constexpr uint32_t trial2_reserved0_60=188u;
inline constexpr uint32_t trial2_reserved0_61=189u;
inline constexpr uint32_t trial2_reserved0_62=190u;
inline constexpr uint32_t trial2_seal=191u;
inline constexpr uint32_t trial4_J=192u;
inline constexpr uint32_t trial4_status=193u;
inline constexpr uint32_t trial4_firstErrorOp=194u;
inline constexpr uint32_t trial4_firstError=195u;
inline constexpr uint32_t trial4_attemptedMicro=196u;
inline constexpr uint32_t trial4_sealedMicro=197u;
inline constexpr uint32_t trial4_selected=198u;
inline constexpr uint32_t trial4_requiredMicro=199u;
inline constexpr uint32_t trial4_activeDispatches=200u;
inline constexpr uint32_t trial4_noopDispatches=201u;
inline constexpr uint32_t trial4_positiveWorkF32=202u;
inline constexpr uint32_t trial4_floatRemainderF32=203u;
inline constexpr uint32_t trial4_baseCommit_lo=204u;
inline constexpr uint32_t trial4_baseCommit_hi=205u;
inline constexpr uint32_t trial4_proposedCommit_lo=206u;
inline constexpr uint32_t trial4_proposedCommit_hi=207u;
inline constexpr uint32_t trial4_inputSHA_0=208u;
inline constexpr uint32_t trial4_inputSHA_1=209u;
inline constexpr uint32_t trial4_inputSHA_2=210u;
inline constexpr uint32_t trial4_inputSHA_3=211u;
inline constexpr uint32_t trial4_inputSHA_4=212u;
inline constexpr uint32_t trial4_inputSHA_5=213u;
inline constexpr uint32_t trial4_inputSHA_6=214u;
inline constexpr uint32_t trial4_inputSHA_7=215u;
inline constexpr uint32_t trial4_fieldSHA_0=216u;
inline constexpr uint32_t trial4_fieldSHA_1=217u;
inline constexpr uint32_t trial4_fieldSHA_2=218u;
inline constexpr uint32_t trial4_fieldSHA_3=219u;
inline constexpr uint32_t trial4_fieldSHA_4=220u;
inline constexpr uint32_t trial4_fieldSHA_5=221u;
inline constexpr uint32_t trial4_fieldSHA_6=222u;
inline constexpr uint32_t trial4_fieldSHA_7=223u;
inline constexpr uint32_t trial4_reserved0_32=224u;
inline constexpr uint32_t trial4_reserved0_33=225u;
inline constexpr uint32_t trial4_reserved0_34=226u;
inline constexpr uint32_t trial4_reserved0_35=227u;
inline constexpr uint32_t trial4_reserved0_36=228u;
inline constexpr uint32_t trial4_reserved0_37=229u;
inline constexpr uint32_t trial4_reserved0_38=230u;
inline constexpr uint32_t trial4_reserved0_39=231u;
inline constexpr uint32_t trial4_reserved0_40=232u;
inline constexpr uint32_t trial4_reserved0_41=233u;
inline constexpr uint32_t trial4_reserved0_42=234u;
inline constexpr uint32_t trial4_reserved0_43=235u;
inline constexpr uint32_t trial4_reserved0_44=236u;
inline constexpr uint32_t trial4_reserved0_45=237u;
inline constexpr uint32_t trial4_reserved0_46=238u;
inline constexpr uint32_t trial4_reserved0_47=239u;
inline constexpr uint32_t trial4_reserved0_48=240u;
inline constexpr uint32_t trial4_reserved0_49=241u;
inline constexpr uint32_t trial4_reserved0_50=242u;
inline constexpr uint32_t trial4_reserved0_51=243u;
inline constexpr uint32_t trial4_reserved0_52=244u;
inline constexpr uint32_t trial4_reserved0_53=245u;
inline constexpr uint32_t trial4_reserved0_54=246u;
inline constexpr uint32_t trial4_reserved0_55=247u;
inline constexpr uint32_t trial4_reserved0_56=248u;
inline constexpr uint32_t trial4_reserved0_57=249u;
inline constexpr uint32_t trial4_reserved0_58=250u;
inline constexpr uint32_t trial4_reserved0_59=251u;
inline constexpr uint32_t trial4_reserved0_60=252u;
inline constexpr uint32_t trial4_reserved0_61=253u;
inline constexpr uint32_t trial4_reserved0_62=254u;
inline constexpr uint32_t trial4_seal=255u;
inline constexpr uint32_t summary_magic=256u;
inline constexpr uint32_t summary_schema=257u;
inline constexpr uint32_t summary_profile=258u;
inline constexpr uint32_t summary_lengthWords=259u;
inline constexpr uint32_t summary_rootKind=260u;
inline constexpr uint32_t summary_outcome=261u;
inline constexpr uint32_t summary_firstErrorOp=262u;
inline constexpr uint32_t summary_firstError=263u;
inline constexpr uint32_t summary_nonce_lo=264u;
inline constexpr uint32_t summary_nonce_hi=265u;
inline constexpr uint32_t summary_plan_lo=266u;
inline constexpr uint32_t summary_plan_hi=267u;
inline constexpr uint32_t summary_run_lo=268u;
inline constexpr uint32_t summary_run_hi=269u;
inline constexpr uint32_t summary_owner_lo=270u;
inline constexpr uint32_t summary_owner_hi=271u;
inline constexpr uint32_t summary_context_lo=272u;
inline constexpr uint32_t summary_context_hi=273u;
inline constexpr uint32_t summary_surface_lo=274u;
inline constexpr uint32_t summary_surface_hi=275u;
inline constexpr uint32_t summary_geometry_lo=276u;
inline constexpr uint32_t summary_geometry_hi=277u;
inline constexpr uint32_t summary_configGeneration_lo=278u;
inline constexpr uint32_t summary_configGeneration_hi=279u;
inline constexpr uint32_t summary_baseCommit_lo=280u;
inline constexpr uint32_t summary_baseCommit_hi=281u;
inline constexpr uint32_t summary_proposedCommit_lo=282u;
inline constexpr uint32_t summary_proposedCommit_hi=283u;
inline constexpr uint32_t summary_startedNativeNs_lo=284u;
inline constexpr uint32_t summary_startedNativeNs_hi=285u;
inline constexpr uint32_t summary_deadlineNativeNs_lo=286u;
inline constexpr uint32_t summary_deadlineNativeNs_hi=287u;
inline constexpr uint32_t summary_completedNativeNs_lo=288u;
inline constexpr uint32_t summary_completedNativeNs_hi=289u;
inline constexpr uint32_t summary_acceptedQuantum_lo=290u;
inline constexpr uint32_t summary_acceptedQuantum_hi=291u;
inline constexpr uint32_t summary_selectedJ=292u;
inline constexpr uint32_t summary_requiredAuditMask=293u;
inline constexpr uint32_t summary_actualAuditMask=294u;
inline constexpr uint32_t summary_actualDispatches=295u;
inline constexpr uint32_t summary_activeDispatches=296u;
inline constexpr uint32_t summary_noopDispatches=297u;
inline constexpr uint32_t summary_publicationEligible=298u;
inline constexpr uint32_t summary_traceSealedMask=299u;
inline constexpr uint32_t summary_consumedSourceSeq_lo=300u;
inline constexpr uint32_t summary_consumedSourceSeq_hi=301u;
inline constexpr uint32_t summary_sourceGeneration_lo=302u;
inline constexpr uint32_t summary_sourceGeneration_hi=303u;
inline constexpr uint32_t summary_configSHA_0=304u;
inline constexpr uint32_t summary_configSHA_1=305u;
inline constexpr uint32_t summary_configSHA_2=306u;
inline constexpr uint32_t summary_configSHA_3=307u;
inline constexpr uint32_t summary_configSHA_4=308u;
inline constexpr uint32_t summary_configSHA_5=309u;
inline constexpr uint32_t summary_configSHA_6=310u;
inline constexpr uint32_t summary_configSHA_7=311u;
inline constexpr uint32_t summary_algorithmSHA_0=312u;
inline constexpr uint32_t summary_algorithmSHA_1=313u;
inline constexpr uint32_t summary_algorithmSHA_2=314u;
inline constexpr uint32_t summary_algorithmSHA_3=315u;
inline constexpr uint32_t summary_algorithmSHA_4=316u;
inline constexpr uint32_t summary_algorithmSHA_5=317u;
inline constexpr uint32_t summary_algorithmSHA_6=318u;
inline constexpr uint32_t summary_algorithmSHA_7=319u;
inline constexpr uint32_t summary_layoutSHA_0=320u;
inline constexpr uint32_t summary_layoutSHA_1=321u;
inline constexpr uint32_t summary_layoutSHA_2=322u;
inline constexpr uint32_t summary_layoutSHA_3=323u;
inline constexpr uint32_t summary_layoutSHA_4=324u;
inline constexpr uint32_t summary_layoutSHA_5=325u;
inline constexpr uint32_t summary_layoutSHA_6=326u;
inline constexpr uint32_t summary_layoutSHA_7=327u;
inline constexpr uint32_t summary_gatesSHA_0=328u;
inline constexpr uint32_t summary_gatesSHA_1=329u;
inline constexpr uint32_t summary_gatesSHA_2=330u;
inline constexpr uint32_t summary_gatesSHA_3=331u;
inline constexpr uint32_t summary_gatesSHA_4=332u;
inline constexpr uint32_t summary_gatesSHA_5=333u;
inline constexpr uint32_t summary_gatesSHA_6=334u;
inline constexpr uint32_t summary_gatesSHA_7=335u;
inline constexpr uint32_t summary_seedSHA_0=336u;
inline constexpr uint32_t summary_seedSHA_1=337u;
inline constexpr uint32_t summary_seedSHA_2=338u;
inline constexpr uint32_t summary_seedSHA_3=339u;
inline constexpr uint32_t summary_seedSHA_4=340u;
inline constexpr uint32_t summary_seedSHA_5=341u;
inline constexpr uint32_t summary_seedSHA_6=342u;
inline constexpr uint32_t summary_seedSHA_7=343u;
inline constexpr uint32_t summary_inputSHA_0=344u;
inline constexpr uint32_t summary_inputSHA_1=345u;
inline constexpr uint32_t summary_inputSHA_2=346u;
inline constexpr uint32_t summary_inputSHA_3=347u;
inline constexpr uint32_t summary_inputSHA_4=348u;
inline constexpr uint32_t summary_inputSHA_5=349u;
inline constexpr uint32_t summary_inputSHA_6=350u;
inline constexpr uint32_t summary_inputSHA_7=351u;
inline constexpr uint32_t summary_fieldSHA_0=352u;
inline constexpr uint32_t summary_fieldSHA_1=353u;
inline constexpr uint32_t summary_fieldSHA_2=354u;
inline constexpr uint32_t summary_fieldSHA_3=355u;
inline constexpr uint32_t summary_fieldSHA_4=356u;
inline constexpr uint32_t summary_fieldSHA_5=357u;
inline constexpr uint32_t summary_fieldSHA_6=358u;
inline constexpr uint32_t summary_fieldSHA_7=359u;
inline constexpr uint32_t summary_rHRMSF32=360u;
inline constexpr uint32_t summary_rHMaxF32=361u;
inline constexpr uint32_t summary_divergenceRMSF32=362u;
inline constexpr uint32_t summary_divergenceMaxF32=363u;
inline constexpr uint32_t summary_pressureCertificateF32=364u;
inline constexpr uint32_t summary_velocityCertificateF32=365u;
inline constexpr uint32_t summary_projectionIdentityMaxF32=366u;
inline constexpr uint32_t summary_RfloatF32=367u;
inline constexpr uint32_t summary_energyBudgetF32=368u;
inline constexpr uint32_t summary_momentumClosureXF32=369u;
inline constexpr uint32_t summary_momentumClosureYF32=370u;
inline constexpr uint32_t summary_positiveWorkF32=371u;
inline constexpr uint32_t summary_renderGeneration_lo=372u;
inline constexpr uint32_t summary_renderGeneration_hi=373u;
inline constexpr uint32_t summary_rootTraceGeneration_lo=374u;
inline constexpr uint32_t summary_rootTraceGeneration_hi=375u;
inline constexpr uint32_t summary_candidateBank=376u;
inline constexpr uint32_t summary_renderPackBank=377u;
inline constexpr uint32_t summary_failureScope=378u;
inline constexpr uint32_t summary_failureTrial=379u;
inline constexpr uint32_t summary_failureMicro=380u;
inline constexpr uint32_t summary_mathBackend=381u;
inline constexpr uint32_t summary_diagnosticMode=382u;
inline constexpr uint32_t summary_precisionFlags=383u;
inline constexpr uint32_t summary_pageSession_lo=384u;
inline constexpr uint32_t summary_pageSession_hi=385u;
inline constexpr uint32_t summary_permission_lo=386u;
inline constexpr uint32_t summary_permission_hi=387u;
inline constexpr uint32_t summary_inputEpoch_lo=388u;
inline constexpr uint32_t summary_inputEpoch_hi=389u;
inline constexpr uint32_t summary_controlGeneration_lo=390u;
inline constexpr uint32_t summary_controlGeneration_hi=391u;
inline constexpr uint32_t summary_thetaWrappedF32=392u;
inline constexpr uint32_t summary_OmegaF32=393u;
inline constexpr uint32_t summary_alphaF32=394u;
inline constexpr uint32_t summary_gxBoxF32=395u;
inline constexpr uint32_t summary_gyBoxF32=396u;
inline constexpr uint32_t summary_motionEnded=397u;
inline constexpr uint32_t summary_forceOn=398u;
inline constexpr uint32_t summary_sourceControlValid=399u;
inline constexpr uint32_t summary_motionOriginQuantum_lo=400u;
inline constexpr uint32_t summary_motionOriginQuantum_hi=401u;
inline constexpr uint32_t summary_reserved0_146=402u;
inline constexpr uint32_t summary_reserved0_147=403u;
inline constexpr uint32_t summary_reserved0_148=404u;
inline constexpr uint32_t summary_reserved0_149=405u;
inline constexpr uint32_t summary_reserved0_150=406u;
inline constexpr uint32_t summary_reserved0_151=407u;
inline constexpr uint32_t summary_reserved0_152=408u;
inline constexpr uint32_t summary_reserved0_153=409u;
inline constexpr uint32_t summary_reserved0_154=410u;
inline constexpr uint32_t summary_reserved0_155=411u;
inline constexpr uint32_t summary_reserved0_156=412u;
inline constexpr uint32_t summary_reserved0_157=413u;
inline constexpr uint32_t summary_reserved0_158=414u;
inline constexpr uint32_t summary_reserved0_159=415u;
inline constexpr uint32_t summary_reserved0_160=416u;
inline constexpr uint32_t summary_reserved0_161=417u;
inline constexpr uint32_t summary_reserved0_162=418u;
inline constexpr uint32_t summary_reserved0_163=419u;
inline constexpr uint32_t summary_reserved0_164=420u;
inline constexpr uint32_t summary_reserved0_165=421u;
inline constexpr uint32_t summary_reserved0_166=422u;
inline constexpr uint32_t summary_reserved0_167=423u;
inline constexpr uint32_t summary_reserved0_168=424u;
inline constexpr uint32_t summary_reserved0_169=425u;
inline constexpr uint32_t summary_reserved0_170=426u;
inline constexpr uint32_t summary_reserved0_171=427u;
inline constexpr uint32_t summary_reserved0_172=428u;
inline constexpr uint32_t summary_reserved0_173=429u;
inline constexpr uint32_t summary_reserved0_174=430u;
inline constexpr uint32_t summary_reserved0_175=431u;
inline constexpr uint32_t summary_reserved0_176=432u;
inline constexpr uint32_t summary_reserved0_177=433u;
inline constexpr uint32_t summary_reserved0_178=434u;
inline constexpr uint32_t summary_reserved0_179=435u;
inline constexpr uint32_t summary_reserved0_180=436u;
inline constexpr uint32_t summary_reserved0_181=437u;
inline constexpr uint32_t summary_reserved0_182=438u;
inline constexpr uint32_t summary_reserved0_183=439u;
inline constexpr uint32_t summary_reserved0_184=440u;
inline constexpr uint32_t summary_reserved0_185=441u;
inline constexpr uint32_t summary_reserved0_186=442u;
inline constexpr uint32_t summary_reserved0_187=443u;
inline constexpr uint32_t summary_reserved0_188=444u;
inline constexpr uint32_t summary_reserved0_189=445u;
inline constexpr uint32_t summary_reserved0_190=446u;
inline constexpr uint32_t summary_reserved0_191=447u;
inline constexpr uint32_t summary_reserved0_192=448u;
inline constexpr uint32_t summary_reserved0_193=449u;
inline constexpr uint32_t summary_reserved0_194=450u;
inline constexpr uint32_t summary_reserved0_195=451u;
inline constexpr uint32_t summary_reserved0_196=452u;
inline constexpr uint32_t summary_reserved0_197=453u;
inline constexpr uint32_t summary_reserved0_198=454u;
inline constexpr uint32_t summary_reserved0_199=455u;
inline constexpr uint32_t summary_reserved0_200=456u;
inline constexpr uint32_t summary_reserved0_201=457u;
inline constexpr uint32_t summary_reserved0_202=458u;
inline constexpr uint32_t summary_reserved0_203=459u;
inline constexpr uint32_t summary_reserved0_204=460u;
inline constexpr uint32_t summary_reserved0_205=461u;
inline constexpr uint32_t summary_reserved0_206=462u;
inline constexpr uint32_t summary_reserved0_207=463u;
inline constexpr uint32_t summary_reserved0_208=464u;
inline constexpr uint32_t summary_reserved0_209=465u;
inline constexpr uint32_t summary_reserved0_210=466u;
inline constexpr uint32_t summary_reserved0_211=467u;
inline constexpr uint32_t summary_reserved0_212=468u;
inline constexpr uint32_t summary_reserved0_213=469u;
inline constexpr uint32_t summary_reserved0_214=470u;
inline constexpr uint32_t summary_reserved0_215=471u;
inline constexpr uint32_t summary_reserved0_216=472u;
inline constexpr uint32_t summary_reserved0_217=473u;
inline constexpr uint32_t summary_reserved0_218=474u;
inline constexpr uint32_t summary_reserved0_219=475u;
inline constexpr uint32_t summary_reserved0_220=476u;
inline constexpr uint32_t summary_reserved0_221=477u;
inline constexpr uint32_t summary_reserved0_222=478u;
inline constexpr uint32_t summary_reserved0_223=479u;
inline constexpr uint32_t summary_reserved0_224=480u;
inline constexpr uint32_t summary_reserved0_225=481u;
inline constexpr uint32_t summary_reserved0_226=482u;
inline constexpr uint32_t summary_reserved0_227=483u;
inline constexpr uint32_t summary_reserved0_228=484u;
inline constexpr uint32_t summary_reserved0_229=485u;
inline constexpr uint32_t summary_reserved0_230=486u;
inline constexpr uint32_t summary_reserved0_231=487u;
inline constexpr uint32_t summary_reserved0_232=488u;
inline constexpr uint32_t summary_reserved0_233=489u;
inline constexpr uint32_t summary_reserved0_234=490u;
inline constexpr uint32_t summary_reserved0_235=491u;
inline constexpr uint32_t summary_reserved0_236=492u;
inline constexpr uint32_t summary_reserved0_237=493u;
inline constexpr uint32_t summary_reserved0_238=494u;
inline constexpr uint32_t summary_reserved0_239=495u;
inline constexpr uint32_t summary_reserved0_240=496u;
inline constexpr uint32_t summary_reserved0_241=497u;
inline constexpr uint32_t summary_reserved0_242=498u;
inline constexpr uint32_t summary_reserved0_243=499u;
inline constexpr uint32_t summary_reserved0_244=500u;
inline constexpr uint32_t summary_reserved0_245=501u;
inline constexpr uint32_t summary_reserved0_246=502u;
inline constexpr uint32_t summary_reserved0_247=503u;
inline constexpr uint32_t summary_reserved0_248=504u;
inline constexpr uint32_t summary_reserved0_249=505u;
inline constexpr uint32_t summary_reserved0_250=506u;
inline constexpr uint32_t summary_reserved0_251=507u;
inline constexpr uint32_t summary_reserved0_252=508u;
inline constexpr uint32_t summary_reserved0_253=509u;
inline constexpr uint32_t summary_reserved0_254=510u;
inline constexpr uint32_t summary_seal=511u;
inline constexpr uint32_t input_magic=512u;
inline constexpr uint32_t input_schema=513u;
inline constexpr uint32_t input_lengthWords=514u;
inline constexpr uint32_t input_kind=515u;
inline constexpr uint32_t input_run_lo=516u;
inline constexpr uint32_t input_run_hi=517u;
inline constexpr uint32_t input_context_lo=518u;
inline constexpr uint32_t input_context_hi=519u;
inline constexpr uint32_t input_surface_lo=520u;
inline constexpr uint32_t input_surface_hi=521u;
inline constexpr uint32_t input_geometry_lo=522u;
inline constexpr uint32_t input_geometry_hi=523u;
inline constexpr uint32_t input_configGeneration_lo=524u;
inline constexpr uint32_t input_configGeneration_hi=525u;
inline constexpr uint32_t input_pageSession_lo=526u;
inline constexpr uint32_t input_pageSession_hi=527u;
inline constexpr uint32_t input_permission_lo=528u;
inline constexpr uint32_t input_permission_hi=529u;
inline constexpr uint32_t input_controlGeneration_lo=530u;
inline constexpr uint32_t input_controlGeneration_hi=531u;
inline constexpr uint32_t input_commandSeq_lo=532u;
inline constexpr uint32_t input_commandSeq_hi=533u;
inline constexpr uint32_t input_sampleNativeNs_lo=534u;
inline constexpr uint32_t input_sampleNativeNs_hi=535u;
inline constexpr uint32_t input_baseCommit_lo=536u;
inline constexpr uint32_t input_baseCommit_hi=537u;
inline constexpr uint32_t input_acceptedQuantum_lo=538u;
inline constexpr uint32_t input_acceptedQuantum_hi=539u;
inline constexpr uint32_t input_motionOriginQuantum_lo=540u;
inline constexpr uint32_t input_motionOriginQuantum_hi=541u;
inline constexpr uint32_t input_diagnosticMode=542u;
inline constexpr uint32_t input_forcedJ=543u;
inline constexpr uint32_t input_configSHA_0=544u;
inline constexpr uint32_t input_configSHA_1=545u;
inline constexpr uint32_t input_configSHA_2=546u;
inline constexpr uint32_t input_configSHA_3=547u;
inline constexpr uint32_t input_configSHA_4=548u;
inline constexpr uint32_t input_configSHA_5=549u;
inline constexpr uint32_t input_configSHA_6=550u;
inline constexpr uint32_t input_configSHA_7=551u;
inline constexpr uint32_t input_algorithmSHA_0=552u;
inline constexpr uint32_t input_algorithmSHA_1=553u;
inline constexpr uint32_t input_algorithmSHA_2=554u;
inline constexpr uint32_t input_algorithmSHA_3=555u;
inline constexpr uint32_t input_algorithmSHA_4=556u;
inline constexpr uint32_t input_algorithmSHA_5=557u;
inline constexpr uint32_t input_algorithmSHA_6=558u;
inline constexpr uint32_t input_algorithmSHA_7=559u;
inline constexpr uint32_t input_layoutSHA_0=560u;
inline constexpr uint32_t input_layoutSHA_1=561u;
inline constexpr uint32_t input_layoutSHA_2=562u;
inline constexpr uint32_t input_layoutSHA_3=563u;
inline constexpr uint32_t input_layoutSHA_4=564u;
inline constexpr uint32_t input_layoutSHA_5=565u;
inline constexpr uint32_t input_layoutSHA_6=566u;
inline constexpr uint32_t input_layoutSHA_7=567u;
inline constexpr uint32_t input_gatesSHA_0=568u;
inline constexpr uint32_t input_gatesSHA_1=569u;
inline constexpr uint32_t input_gatesSHA_2=570u;
inline constexpr uint32_t input_gatesSHA_3=571u;
inline constexpr uint32_t input_gatesSHA_4=572u;
inline constexpr uint32_t input_gatesSHA_5=573u;
inline constexpr uint32_t input_gatesSHA_6=574u;
inline constexpr uint32_t input_gatesSHA_7=575u;
inline constexpr uint32_t input_seedSHA_0=576u;
inline constexpr uint32_t input_seedSHA_1=577u;
inline constexpr uint32_t input_seedSHA_2=578u;
inline constexpr uint32_t input_seedSHA_3=579u;
inline constexpr uint32_t input_seedSHA_4=580u;
inline constexpr uint32_t input_seedSHA_5=581u;
inline constexpr uint32_t input_seedSHA_6=582u;
inline constexpr uint32_t input_seedSHA_7=583u;
inline constexpr uint32_t input_fixtureSHA_0=584u;
inline constexpr uint32_t input_fixtureSHA_1=585u;
inline constexpr uint32_t input_fixtureSHA_2=586u;
inline constexpr uint32_t input_fixtureSHA_3=587u;
inline constexpr uint32_t input_fixtureSHA_4=588u;
inline constexpr uint32_t input_fixtureSHA_5=589u;
inline constexpr uint32_t input_fixtureSHA_6=590u;
inline constexpr uint32_t input_fixtureSHA_7=591u;
inline constexpr uint32_t input_inputSHA_0=592u;
inline constexpr uint32_t input_inputSHA_1=593u;
inline constexpr uint32_t input_inputSHA_2=594u;
inline constexpr uint32_t input_inputSHA_3=595u;
inline constexpr uint32_t input_inputSHA_4=596u;
inline constexpr uint32_t input_inputSHA_5=597u;
inline constexpr uint32_t input_inputSHA_6=598u;
inline constexpr uint32_t input_inputSHA_7=599u;
inline constexpr uint32_t input_sourceOpcode=600u;
inline constexpr uint32_t input_motionMode=601u;
inline constexpr uint32_t input_theta0F32=602u;
inline constexpr uint32_t input_OmF32=603u;
inline constexpr uint32_t input_durationF32=604u;
inline constexpr uint32_t input_gxWorldF32=605u;
inline constexpr uint32_t input_gyWorldF32=606u;
inline constexpr uint32_t input_forceKind=607u;
inline constexpr uint32_t input_forceAmpF32=608u;
inline constexpr uint32_t input_forceStartQuantum_lo=609u;
inline constexpr uint32_t input_forceStartQuantum_hi=610u;
inline constexpr uint32_t input_forceEndQuantum_lo=611u;
inline constexpr uint32_t input_forceEndQuantum_hi=612u;
inline constexpr uint32_t input_wallBottomF32=613u;
inline constexpr uint32_t input_wallTopF32=614u;
inline constexpr uint32_t input_boundaryRevision=615u;
inline constexpr uint32_t input_sourceIntent_lo=616u;
inline constexpr uint32_t input_sourceIntent_hi=617u;
inline constexpr uint32_t input_enqueueSerial_lo=618u;
inline constexpr uint32_t input_enqueueSerial_hi=619u;
inline constexpr uint32_t input_reserved0_108=620u;
inline constexpr uint32_t input_reserved0_109=621u;
inline constexpr uint32_t input_reserved0_110=622u;
inline constexpr uint32_t input_reserved0_111=623u;
inline constexpr uint32_t input_reserved0_112=624u;
inline constexpr uint32_t input_reserved0_113=625u;
inline constexpr uint32_t input_reserved0_114=626u;
inline constexpr uint32_t input_reserved0_115=627u;
inline constexpr uint32_t input_reserved0_116=628u;
inline constexpr uint32_t input_reserved0_117=629u;
inline constexpr uint32_t input_reserved0_118=630u;
inline constexpr uint32_t input_reserved0_119=631u;
inline constexpr uint32_t input_reserved0_120=632u;
inline constexpr uint32_t input_reserved0_121=633u;
inline constexpr uint32_t input_reserved0_122=634u;
inline constexpr uint32_t input_reserved0_123=635u;
inline constexpr uint32_t input_reserved0_124=636u;
inline constexpr uint32_t input_reserved0_125=637u;
inline constexpr uint32_t input_reserved0_126=638u;
inline constexpr uint32_t input_seal=639u;
inline constexpr uint32_t config_magic=640u;
inline constexpr uint32_t config_schema=641u;
inline constexpr uint32_t config_profile=642u;
inline constexpr uint32_t config_lengthWords=643u;
inline constexpr uint32_t config_Nx=644u;
inline constexpr uint32_t config_Ny=645u;
inline constexpr uint32_t config_boundary=646u;
inline constexpr uint32_t config_seedMode=647u;
inline constexpr uint32_t config_LxF32=648u;
inline constexpr uint32_t config_LyF32=649u;
inline constexpr uint32_t config_hF32=650u;
inline constexpr uint32_t config_T0F32=651u;
inline constexpr uint32_t config_L0Binary64_lo=652u;
inline constexpr uint32_t config_L0Binary64_hi=653u;
inline constexpr uint32_t config_rho0Binary64_lo=654u;
inline constexpr uint32_t config_rho0Binary64_hi=655u;
inline constexpr uint32_t config_muSIBinary64_lo=656u;
inline constexpr uint32_t config_muSIBinary64_hi=657u;
inline constexpr uint32_t config_nuF32=658u;
inline constexpr uint32_t config_unitPolicy=659u;
inline constexpr uint32_t config_motionMode=660u;
inline constexpr uint32_t config_OmF32=661u;
inline constexpr uint32_t config_durationF32=662u;
inline constexpr uint32_t config_theta0F32=663u;
inline constexpr uint32_t config_gxWorldF32=664u;
inline constexpr uint32_t config_gyWorldF32=665u;
inline constexpr uint32_t config_forceAmpF32=666u;
inline constexpr uint32_t config_forceKind=667u;
inline constexpr uint32_t config_tracerCount=668u;
inline constexpr uint32_t config_positiveWorkCapF32=669u;
inline constexpr uint32_t config_precisionPolicy=670u;
inline constexpr uint32_t config_gatesRevision=671u;
inline constexpr uint32_t config_wallBottomF32=672u;
inline constexpr uint32_t config_wallTopF32=673u;
inline constexpr uint32_t config_pressureReferenceF32=674u;
inline constexpr uint32_t config_reserved0_35=675u;
inline constexpr uint32_t config_algorithmSHA_0=676u;
inline constexpr uint32_t config_algorithmSHA_1=677u;
inline constexpr uint32_t config_algorithmSHA_2=678u;
inline constexpr uint32_t config_algorithmSHA_3=679u;
inline constexpr uint32_t config_algorithmSHA_4=680u;
inline constexpr uint32_t config_algorithmSHA_5=681u;
inline constexpr uint32_t config_algorithmSHA_6=682u;
inline constexpr uint32_t config_algorithmSHA_7=683u;
inline constexpr uint32_t config_layoutSHA_0=684u;
inline constexpr uint32_t config_layoutSHA_1=685u;
inline constexpr uint32_t config_layoutSHA_2=686u;
inline constexpr uint32_t config_layoutSHA_3=687u;
inline constexpr uint32_t config_layoutSHA_4=688u;
inline constexpr uint32_t config_layoutSHA_5=689u;
inline constexpr uint32_t config_layoutSHA_6=690u;
inline constexpr uint32_t config_layoutSHA_7=691u;
inline constexpr uint32_t config_gatesSHA_0=692u;
inline constexpr uint32_t config_gatesSHA_1=693u;
inline constexpr uint32_t config_gatesSHA_2=694u;
inline constexpr uint32_t config_gatesSHA_3=695u;
inline constexpr uint32_t config_gatesSHA_4=696u;
inline constexpr uint32_t config_gatesSHA_5=697u;
inline constexpr uint32_t config_gatesSHA_6=698u;
inline constexpr uint32_t config_gatesSHA_7=699u;
inline constexpr uint32_t config_seedSHA_0=700u;
inline constexpr uint32_t config_seedSHA_1=701u;
inline constexpr uint32_t config_seedSHA_2=702u;
inline constexpr uint32_t config_seedSHA_3=703u;
inline constexpr uint32_t config_seedSHA_4=704u;
inline constexpr uint32_t config_seedSHA_5=705u;
inline constexpr uint32_t config_seedSHA_6=706u;
inline constexpr uint32_t config_seedSHA_7=707u;
inline constexpr uint32_t config_fixtureSHA_0=708u;
inline constexpr uint32_t config_fixtureSHA_1=709u;
inline constexpr uint32_t config_fixtureSHA_2=710u;
inline constexpr uint32_t config_fixtureSHA_3=711u;
inline constexpr uint32_t config_fixtureSHA_4=712u;
inline constexpr uint32_t config_fixtureSHA_5=713u;
inline constexpr uint32_t config_fixtureSHA_6=714u;
inline constexpr uint32_t config_fixtureSHA_7=715u;
inline constexpr uint32_t config_configSHA_0=716u;
inline constexpr uint32_t config_configSHA_1=717u;
inline constexpr uint32_t config_configSHA_2=718u;
inline constexpr uint32_t config_configSHA_3=719u;
inline constexpr uint32_t config_configSHA_4=720u;
inline constexpr uint32_t config_configSHA_5=721u;
inline constexpr uint32_t config_configSHA_6=722u;
inline constexpr uint32_t config_configSHA_7=723u;
inline constexpr uint32_t config_reserved0_84=724u;
inline constexpr uint32_t config_reserved0_85=725u;
inline constexpr uint32_t config_reserved0_86=726u;
inline constexpr uint32_t config_reserved0_87=727u;
inline constexpr uint32_t config_reserved0_88=728u;
inline constexpr uint32_t config_reserved0_89=729u;
inline constexpr uint32_t config_reserved0_90=730u;
inline constexpr uint32_t config_reserved0_91=731u;
inline constexpr uint32_t config_reserved0_92=732u;
inline constexpr uint32_t config_reserved0_93=733u;
inline constexpr uint32_t config_reserved0_94=734u;
inline constexpr uint32_t config_reserved0_95=735u;
inline constexpr uint32_t config_reserved0_96=736u;
inline constexpr uint32_t config_reserved0_97=737u;
inline constexpr uint32_t config_reserved0_98=738u;
inline constexpr uint32_t config_reserved0_99=739u;
inline constexpr uint32_t config_reserved0_100=740u;
inline constexpr uint32_t config_reserved0_101=741u;
inline constexpr uint32_t config_reserved0_102=742u;
inline constexpr uint32_t config_reserved0_103=743u;
inline constexpr uint32_t config_reserved0_104=744u;
inline constexpr uint32_t config_reserved0_105=745u;
inline constexpr uint32_t config_reserved0_106=746u;
inline constexpr uint32_t config_reserved0_107=747u;
inline constexpr uint32_t config_reserved0_108=748u;
inline constexpr uint32_t config_reserved0_109=749u;
inline constexpr uint32_t config_reserved0_110=750u;
inline constexpr uint32_t config_reserved0_111=751u;
inline constexpr uint32_t config_reserved0_112=752u;
inline constexpr uint32_t config_reserved0_113=753u;
inline constexpr uint32_t config_reserved0_114=754u;
inline constexpr uint32_t config_reserved0_115=755u;
inline constexpr uint32_t config_reserved0_116=756u;
inline constexpr uint32_t config_reserved0_117=757u;
inline constexpr uint32_t config_reserved0_118=758u;
inline constexpr uint32_t config_reserved0_119=759u;
inline constexpr uint32_t config_reserved0_120=760u;
inline constexpr uint32_t config_reserved0_121=761u;
inline constexpr uint32_t config_reserved0_122=762u;
inline constexpr uint32_t config_reserved0_123=763u;
inline constexpr uint32_t config_reserved0_124=764u;
inline constexpr uint32_t config_reserved0_125=765u;
inline constexpr uint32_t config_reserved0_126=766u;
inline constexpr uint32_t config_reserved0_127=767u;
inline constexpr uint32_t mathQualification_schema=768u;
inline constexpr uint32_t mathQualification_backend=769u;
inline constexpr uint32_t mathQualification_qualificationFlags=770u;
inline constexpr uint32_t mathQualification_sampleCount=771u;
inline constexpr uint32_t mathQualification_mathSHA_0=772u;
inline constexpr uint32_t mathQualification_mathSHA_1=773u;
inline constexpr uint32_t mathQualification_mathSHA_2=774u;
inline constexpr uint32_t mathQualification_mathSHA_3=775u;
inline constexpr uint32_t mathQualification_mathSHA_4=776u;
inline constexpr uint32_t mathQualification_mathSHA_5=777u;
inline constexpr uint32_t mathQualification_mathSHA_6=778u;
inline constexpr uint32_t mathQualification_mathSHA_7=779u;
inline constexpr uint32_t mathQualification_programSHA_0=780u;
inline constexpr uint32_t mathQualification_programSHA_1=781u;
inline constexpr uint32_t mathQualification_programSHA_2=782u;
inline constexpr uint32_t mathQualification_programSHA_3=783u;
inline constexpr uint32_t mathQualification_programSHA_4=784u;
inline constexpr uint32_t mathQualification_programSHA_5=785u;
inline constexpr uint32_t mathQualification_programSHA_6=786u;
inline constexpr uint32_t mathQualification_programSHA_7=787u;
inline constexpr uint32_t mathQualification_deviceBuildSHA_0=788u;
inline constexpr uint32_t mathQualification_deviceBuildSHA_1=789u;
inline constexpr uint32_t mathQualification_deviceBuildSHA_2=790u;
inline constexpr uint32_t mathQualification_deviceBuildSHA_3=791u;
inline constexpr uint32_t mathQualification_deviceBuildSHA_4=792u;
inline constexpr uint32_t mathQualification_deviceBuildSHA_5=793u;
inline constexpr uint32_t mathQualification_deviceBuildSHA_6=794u;
inline constexpr uint32_t mathQualification_deviceBuildSHA_7=795u;
inline constexpr uint32_t mathQualification_reserved0_28=796u;
inline constexpr uint32_t mathQualification_reserved0_29=797u;
inline constexpr uint32_t mathQualification_reserved0_30=798u;
inline constexpr uint32_t mathQualification_reserved0_31=799u;
inline constexpr uint32_t mathQualification_sample0_functionId=800u;
inline constexpr uint32_t mathQualification_sample0_argumentBinary64Lo=801u;
inline constexpr uint32_t mathQualification_sample0_argumentBinary64Hi=802u;
inline constexpr uint32_t mathQualification_sample0_resultF32=803u;
inline constexpr uint32_t mathQualification_sample0_referenceF32=804u;
inline constexpr uint32_t mathQualification_sample0_errorBoundF32=805u;
inline constexpr uint32_t mathQualification_sample1_functionId=806u;
inline constexpr uint32_t mathQualification_sample1_argumentBinary64Lo=807u;
inline constexpr uint32_t mathQualification_sample1_argumentBinary64Hi=808u;
inline constexpr uint32_t mathQualification_sample1_resultF32=809u;
inline constexpr uint32_t mathQualification_sample1_referenceF32=810u;
inline constexpr uint32_t mathQualification_sample1_errorBoundF32=811u;
inline constexpr uint32_t mathQualification_sample2_functionId=812u;
inline constexpr uint32_t mathQualification_sample2_argumentBinary64Lo=813u;
inline constexpr uint32_t mathQualification_sample2_argumentBinary64Hi=814u;
inline constexpr uint32_t mathQualification_sample2_resultF32=815u;
inline constexpr uint32_t mathQualification_sample2_referenceF32=816u;
inline constexpr uint32_t mathQualification_sample2_errorBoundF32=817u;
inline constexpr uint32_t mathQualification_sample3_functionId=818u;
inline constexpr uint32_t mathQualification_sample3_argumentBinary64Lo=819u;
inline constexpr uint32_t mathQualification_sample3_argumentBinary64Hi=820u;
inline constexpr uint32_t mathQualification_sample3_resultF32=821u;
inline constexpr uint32_t mathQualification_sample3_referenceF32=822u;
inline constexpr uint32_t mathQualification_sample3_errorBoundF32=823u;
inline constexpr uint32_t mathQualification_sample4_functionId=824u;
inline constexpr uint32_t mathQualification_sample4_argumentBinary64Lo=825u;
inline constexpr uint32_t mathQualification_sample4_argumentBinary64Hi=826u;
inline constexpr uint32_t mathQualification_sample4_resultF32=827u;
inline constexpr uint32_t mathQualification_sample4_referenceF32=828u;
inline constexpr uint32_t mathQualification_sample4_errorBoundF32=829u;
inline constexpr uint32_t mathQualification_sample5_functionId=830u;
inline constexpr uint32_t mathQualification_sample5_argumentBinary64Lo=831u;
inline constexpr uint32_t mathQualification_sample5_argumentBinary64Hi=832u;
inline constexpr uint32_t mathQualification_sample5_resultF32=833u;
inline constexpr uint32_t mathQualification_sample5_referenceF32=834u;
inline constexpr uint32_t mathQualification_sample5_errorBoundF32=835u;
inline constexpr uint32_t mathQualification_sample6_functionId=836u;
inline constexpr uint32_t mathQualification_sample6_argumentBinary64Lo=837u;
inline constexpr uint32_t mathQualification_sample6_argumentBinary64Hi=838u;
inline constexpr uint32_t mathQualification_sample6_resultF32=839u;
inline constexpr uint32_t mathQualification_sample6_referenceF32=840u;
inline constexpr uint32_t mathQualification_sample6_errorBoundF32=841u;
inline constexpr uint32_t mathQualification_sample7_functionId=842u;
inline constexpr uint32_t mathQualification_sample7_argumentBinary64Lo=843u;
inline constexpr uint32_t mathQualification_sample7_argumentBinary64Hi=844u;
inline constexpr uint32_t mathQualification_sample7_resultF32=845u;
inline constexpr uint32_t mathQualification_sample7_referenceF32=846u;
inline constexpr uint32_t mathQualification_sample7_errorBoundF32=847u;
inline constexpr uint32_t mathQualification_sample8_functionId=848u;
inline constexpr uint32_t mathQualification_sample8_argumentBinary64Lo=849u;
inline constexpr uint32_t mathQualification_sample8_argumentBinary64Hi=850u;
inline constexpr uint32_t mathQualification_sample8_resultF32=851u;
inline constexpr uint32_t mathQualification_sample8_referenceF32=852u;
inline constexpr uint32_t mathQualification_sample8_errorBoundF32=853u;
inline constexpr uint32_t mathQualification_sample9_functionId=854u;
inline constexpr uint32_t mathQualification_sample9_argumentBinary64Lo=855u;
inline constexpr uint32_t mathQualification_sample9_argumentBinary64Hi=856u;
inline constexpr uint32_t mathQualification_sample9_resultF32=857u;
inline constexpr uint32_t mathQualification_sample9_referenceF32=858u;
inline constexpr uint32_t mathQualification_sample9_errorBoundF32=859u;
inline constexpr uint32_t mathQualification_sample10_functionId=860u;
inline constexpr uint32_t mathQualification_sample10_argumentBinary64Lo=861u;
inline constexpr uint32_t mathQualification_sample10_argumentBinary64Hi=862u;
inline constexpr uint32_t mathQualification_sample10_resultF32=863u;
inline constexpr uint32_t mathQualification_sample10_referenceF32=864u;
inline constexpr uint32_t mathQualification_sample10_errorBoundF32=865u;
inline constexpr uint32_t mathQualification_sample11_functionId=866u;
inline constexpr uint32_t mathQualification_sample11_argumentBinary64Lo=867u;
inline constexpr uint32_t mathQualification_sample11_argumentBinary64Hi=868u;
inline constexpr uint32_t mathQualification_sample11_resultF32=869u;
inline constexpr uint32_t mathQualification_sample11_referenceF32=870u;
inline constexpr uint32_t mathQualification_sample11_errorBoundF32=871u;
inline constexpr uint32_t mathQualification_sample12_functionId=872u;
inline constexpr uint32_t mathQualification_sample12_argumentBinary64Lo=873u;
inline constexpr uint32_t mathQualification_sample12_argumentBinary64Hi=874u;
inline constexpr uint32_t mathQualification_sample12_resultF32=875u;
inline constexpr uint32_t mathQualification_sample12_referenceF32=876u;
inline constexpr uint32_t mathQualification_sample12_errorBoundF32=877u;
inline constexpr uint32_t mathQualification_sample13_functionId=878u;
inline constexpr uint32_t mathQualification_sample13_argumentBinary64Lo=879u;
inline constexpr uint32_t mathQualification_sample13_argumentBinary64Hi=880u;
inline constexpr uint32_t mathQualification_sample13_resultF32=881u;
inline constexpr uint32_t mathQualification_sample13_referenceF32=882u;
inline constexpr uint32_t mathQualification_sample13_errorBoundF32=883u;
inline constexpr uint32_t mathQualification_sample14_functionId=884u;
inline constexpr uint32_t mathQualification_sample14_argumentBinary64Lo=885u;
inline constexpr uint32_t mathQualification_sample14_argumentBinary64Hi=886u;
inline constexpr uint32_t mathQualification_sample14_resultF32=887u;
inline constexpr uint32_t mathQualification_sample14_referenceF32=888u;
inline constexpr uint32_t mathQualification_sample14_errorBoundF32=889u;
inline constexpr uint32_t mathQualification_sample15_functionId=890u;
inline constexpr uint32_t mathQualification_sample15_argumentBinary64Lo=891u;
inline constexpr uint32_t mathQualification_sample15_argumentBinary64Hi=892u;
inline constexpr uint32_t mathQualification_sample15_resultF32=893u;
inline constexpr uint32_t mathQualification_sample15_referenceF32=894u;
inline constexpr uint32_t mathQualification_sample15_errorBoundF32=895u;
inline constexpr uint32_t quotaAndClock_session_lo=896u;
inline constexpr uint32_t quotaAndClock_session_hi=897u;
inline constexpr uint32_t quotaAndClock_sequence_lo=898u;
inline constexpr uint32_t quotaAndClock_sequence_hi=899u;
inline constexpr uint32_t quotaAndClock_uiNs_lo=900u;
inline constexpr uint32_t quotaAndClock_uiNs_hi=901u;
inline constexpr uint32_t quotaAndClock_nativeReceiveNs_lo=902u;
inline constexpr uint32_t quotaAndClock_nativeReceiveNs_hi=903u;
inline constexpr uint32_t quotaAndClock_pulses_lo=904u;
inline constexpr uint32_t quotaAndClock_pulses_hi=905u;
inline constexpr uint32_t quotaAndClock_maxUiGapNs_lo=906u;
inline constexpr uint32_t quotaAndClock_maxUiGapNs_hi=907u;
inline constexpr uint32_t quotaAndClock_maxNativeGapNs_lo=908u;
inline constexpr uint32_t quotaAndClock_maxNativeGapNs_hi=909u;
inline constexpr uint32_t quotaAndClock_generated_lo=910u;
inline constexpr uint32_t quotaAndClock_generated_hi=911u;
inline constexpr uint32_t quotaAndClock_accepted_lo=912u;
inline constexpr uint32_t quotaAndClock_accepted_hi=913u;
inline constexpr uint32_t quotaAndClock_overload_lo=914u;
inline constexpr uint32_t quotaAndClock_overload_hi=915u;
inline constexpr uint32_t quotaAndClock_revoked_lo=916u;
inline constexpr uint32_t quotaAndClock_revoked_hi=917u;
inline constexpr uint32_t quotaAndClock_carry_lo=918u;
inline constexpr uint32_t quotaAndClock_carry_hi=919u;
inline constexpr uint32_t quotaAndClock_clearedCarry_lo=920u;
inline constexpr uint32_t quotaAndClock_clearedCarry_hi=921u;
inline constexpr uint32_t quotaAndClock_planSerial_lo=922u;
inline constexpr uint32_t quotaAndClock_planSerial_hi=923u;
inline constexpr uint32_t quotaAndClock_pending_lo=924u;
inline constexpr uint32_t quotaAndClock_pending_hi=925u;
inline constexpr uint32_t quotaAndClock_activeRemaining_lo=926u;
inline constexpr uint32_t quotaAndClock_activeRemaining_hi=927u;
inline constexpr uint32_t quotaAndClock_sourceBoundary_lo=928u;
inline constexpr uint32_t quotaAndClock_sourceBoundary_hi=929u;
inline constexpr uint32_t quotaAndClock_sourceIntent_lo=930u;
inline constexpr uint32_t quotaAndClock_sourceIntent_hi=931u;
inline constexpr uint32_t quotaAndClock_sourceControlGeneration_lo=932u;
inline constexpr uint32_t quotaAndClock_sourceControlGeneration_hi=933u;
inline constexpr uint32_t quotaAndClock_meaningfulInputSeq_lo=934u;
inline constexpr uint32_t quotaAndClock_meaningfulInputSeq_hi=935u;
inline constexpr uint32_t quotaAndClock_retiredPlan_lo=936u;
inline constexpr uint32_t quotaAndClock_retiredPlan_hi=937u;
inline constexpr uint32_t quotaAndClock_retiredNonce_lo=938u;
inline constexpr uint32_t quotaAndClock_retiredNonce_hi=939u;
inline constexpr uint32_t quotaAndClock_nativePollNs_lo=940u;
inline constexpr uint32_t quotaAndClock_nativePollNs_hi=941u;
inline constexpr uint32_t quotaAndClock_rootSubmitNs_lo=942u;
inline constexpr uint32_t quotaAndClock_rootSubmitNs_hi=943u;
inline constexpr uint32_t quotaAndClock_rootDeadlineNs_lo=944u;
inline constexpr uint32_t quotaAndClock_rootDeadlineNs_hi=945u;
inline constexpr uint32_t quotaAndClock_rootCompletionPollNs_lo=946u;
inline constexpr uint32_t quotaAndClock_rootCompletionPollNs_hi=947u;
inline constexpr uint32_t quotaAndClock_sceneNewQuantumFrames_lo=948u;
inline constexpr uint32_t quotaAndClock_sceneNewQuantumFrames_hi=949u;
inline constexpr uint32_t quotaAndClock_sceneSourceFrames_lo=950u;
inline constexpr uint32_t quotaAndClock_sceneSourceFrames_hi=951u;
inline constexpr uint32_t quotaAndClock_sceneRepeatFrames_lo=952u;
inline constexpr uint32_t quotaAndClock_sceneRepeatFrames_hi=953u;
inline constexpr uint32_t quotaAndClock_sceneStaleFrames_lo=954u;
inline constexpr uint32_t quotaAndClock_sceneStaleFrames_hi=955u;
inline constexpr uint32_t quotaAndClock_clockFault_lo=956u;
inline constexpr uint32_t quotaAndClock_clockFault_hi=957u;
inline constexpr uint32_t quotaAndClock_flags_lo=958u;
inline constexpr uint32_t quotaAndClock_flags_hi=959u;
inline constexpr uint32_t reserved0_960=960u;
inline constexpr uint32_t reserved0_961=961u;
inline constexpr uint32_t reserved0_962=962u;
inline constexpr uint32_t reserved0_963=963u;
inline constexpr uint32_t reserved0_964=964u;
inline constexpr uint32_t reserved0_965=965u;
inline constexpr uint32_t reserved0_966=966u;
inline constexpr uint32_t reserved0_967=967u;
inline constexpr uint32_t reserved0_968=968u;
inline constexpr uint32_t reserved0_969=969u;
inline constexpr uint32_t reserved0_970=970u;
inline constexpr uint32_t reserved0_971=971u;
inline constexpr uint32_t reserved0_972=972u;
inline constexpr uint32_t reserved0_973=973u;
inline constexpr uint32_t reserved0_974=974u;
inline constexpr uint32_t reserved0_975=975u;
inline constexpr uint32_t reserved0_976=976u;
inline constexpr uint32_t reserved0_977=977u;
inline constexpr uint32_t reserved0_978=978u;
inline constexpr uint32_t reserved0_979=979u;
inline constexpr uint32_t reserved0_980=980u;
inline constexpr uint32_t reserved0_981=981u;
inline constexpr uint32_t reserved0_982=982u;
inline constexpr uint32_t reserved0_983=983u;
inline constexpr uint32_t reserved0_984=984u;
inline constexpr uint32_t reserved0_985=985u;
inline constexpr uint32_t reserved0_986=986u;
inline constexpr uint32_t reserved0_987=987u;
inline constexpr uint32_t reserved0_988=988u;
inline constexpr uint32_t reserved0_989=989u;
inline constexpr uint32_t reserved0_990=990u;
inline constexpr uint32_t reserved0_991=991u;
inline constexpr uint32_t reserved0_992=992u;
inline constexpr uint32_t reserved0_993=993u;
inline constexpr uint32_t reserved0_994=994u;
inline constexpr uint32_t reserved0_995=995u;
inline constexpr uint32_t reserved0_996=996u;
inline constexpr uint32_t reserved0_997=997u;
inline constexpr uint32_t reserved0_998=998u;
inline constexpr uint32_t reserved0_999=999u;
inline constexpr uint32_t reserved0_1000=1000u;
inline constexpr uint32_t reserved0_1001=1001u;
inline constexpr uint32_t reserved0_1002=1002u;
inline constexpr uint32_t reserved0_1003=1003u;
inline constexpr uint32_t reserved0_1004=1004u;
inline constexpr uint32_t reserved0_1005=1005u;
inline constexpr uint32_t reserved0_1006=1006u;
inline constexpr uint32_t reserved0_1007=1007u;
inline constexpr uint32_t reserved0_1008=1008u;
inline constexpr uint32_t reserved0_1009=1009u;
inline constexpr uint32_t reserved0_1010=1010u;
inline constexpr uint32_t reserved0_1011=1011u;
inline constexpr uint32_t reserved0_1012=1012u;
inline constexpr uint32_t reserved0_1013=1013u;
inline constexpr uint32_t reserved0_1014=1014u;
inline constexpr uint32_t reserved0_1015=1015u;
inline constexpr uint32_t reserved0_1016=1016u;
inline constexpr uint32_t reserved0_1017=1017u;
inline constexpr uint32_t reserved0_1018=1018u;
inline constexpr uint32_t reserved0_1019=1019u;
inline constexpr uint32_t reserved0_1020=1020u;
inline constexpr uint32_t reserved0_1021=1021u;
inline constexpr uint32_t reserved0_1022=1022u;
inline constexpr uint32_t seal=1023u;
}
namespace RenderCell16 {
inline constexpr uint32_t uCenterF32=0u;
inline constexpr uint32_t vCenterF32=1u;
inline constexpr uint32_t piF32=2u;
inline constexpr uint32_t phiF32=3u;
inline constexpr uint32_t totalPressureF32=4u;
inline constexpr uint32_t curlF32=5u;
inline constexpr uint32_t divergenceF32=6u;
inline constexpr uint32_t speedF32=7u;
inline constexpr uint32_t xF32=8u;
inline constexpr uint32_t yF32=9u;
inline constexpr uint32_t viscResidualProxyF32=10u;
inline constexpr uint32_t pressureResidualF32=11u;
inline constexpr uint32_t validFlags=12u;
inline constexpr uint32_t cellId=13u;
inline constexpr uint32_t boundaryFlags=14u;
inline constexpr uint32_t reserved0=15u;
}
namespace Tracer8 {
inline constexpr uint32_t xF32=0u;
inline constexpr uint32_t yF32=1u;
inline constexpr uint32_t previousXF32=2u;
inline constexpr uint32_t previousYF32=3u;
inline constexpr uint32_t seedId=4u;
inline constexpr uint32_t flags=5u;
inline constexpr uint32_t ageTicks480Hz=6u;
inline constexpr uint32_t reservedZero=7u;
}
namespace RenderHeader64 {
inline constexpr uint32_t magic=0u;
inline constexpr uint32_t schema=1u;
inline constexpr uint32_t profile=2u;
inline constexpr uint32_t lengthWords=3u;
inline constexpr uint32_t Nx=4u;
inline constexpr uint32_t Ny=5u;
inline constexpr uint32_t boundary=6u;
inline constexpr uint32_t flags=7u;
inline constexpr uint32_t run_lo=8u;
inline constexpr uint32_t run_hi=9u;
inline constexpr uint32_t context_lo=10u;
inline constexpr uint32_t context_hi=11u;
inline constexpr uint32_t surface_lo=12u;
inline constexpr uint32_t surface_hi=13u;
inline constexpr uint32_t geometry_lo=14u;
inline constexpr uint32_t geometry_hi=15u;
inline constexpr uint32_t commit_lo=16u;
inline constexpr uint32_t commit_hi=17u;
inline constexpr uint32_t nonce_lo=18u;
inline constexpr uint32_t nonce_hi=19u;
inline constexpr uint32_t packGeneration_lo=20u;
inline constexpr uint32_t packGeneration_hi=21u;
inline constexpr uint32_t configGeneration_lo=22u;
inline constexpr uint32_t configGeneration_hi=23u;
inline constexpr uint32_t configSHA_0=24u;
inline constexpr uint32_t configSHA_1=25u;
inline constexpr uint32_t configSHA_2=26u;
inline constexpr uint32_t configSHA_3=27u;
inline constexpr uint32_t configSHA_4=28u;
inline constexpr uint32_t configSHA_5=29u;
inline constexpr uint32_t configSHA_6=30u;
inline constexpr uint32_t configSHA_7=31u;
inline constexpr uint32_t layoutSHA_0=32u;
inline constexpr uint32_t layoutSHA_1=33u;
inline constexpr uint32_t layoutSHA_2=34u;
inline constexpr uint32_t layoutSHA_3=35u;
inline constexpr uint32_t layoutSHA_4=36u;
inline constexpr uint32_t layoutSHA_5=37u;
inline constexpr uint32_t layoutSHA_6=38u;
inline constexpr uint32_t layoutSHA_7=39u;
inline constexpr uint32_t uColorMinF32=40u;
inline constexpr uint32_t uColorMaxF32=41u;
inline constexpr uint32_t piColorMinF32=42u;
inline constexpr uint32_t piColorMaxF32=43u;
inline constexpr uint32_t curlColorMinF32=44u;
inline constexpr uint32_t curlColorMaxF32=45u;
inline constexpr uint32_t thetaWrappedF32=46u;
inline constexpr uint32_t phiMeanF32=47u;
inline constexpr uint32_t pressureReferenceF32=48u;
inline constexpr uint32_t displayMode=49u;
inline constexpr uint32_t reserved0_50=50u;
inline constexpr uint32_t reserved0_51=51u;
inline constexpr uint32_t reserved0_52=52u;
inline constexpr uint32_t reserved0_53=53u;
inline constexpr uint32_t reserved0_54=54u;
inline constexpr uint32_t reserved0_55=55u;
inline constexpr uint32_t reserved0_56=56u;
inline constexpr uint32_t reserved0_57=57u;
inline constexpr uint32_t reserved0_58=58u;
inline constexpr uint32_t reserved0_59=59u;
inline constexpr uint32_t reserved0_60=60u;
inline constexpr uint32_t reserved0_61=61u;
inline constexpr uint32_t reserved0_62=62u;
inline constexpr uint32_t seal=63u;
}
namespace ColdDescriptor32 {
inline constexpr uint32_t kind=0u;
inline constexpr uint32_t schema=1u;
inline constexpr uint32_t segmentIndex=2u;
inline constexpr uint32_t flags=3u;
inline constexpr uint32_t bankRole=4u;
inline constexpr uint32_t strideWords=5u;
inline constexpr uint32_t byteOffset_lo=6u;
inline constexpr uint32_t byteOffset_hi=7u;
inline constexpr uint32_t byteLength_lo=8u;
inline constexpr uint32_t byteLength_hi=9u;
inline constexpr uint32_t validRows=10u;
inline constexpr uint32_t plannedRows=11u;
inline constexpr uint32_t sealedRows=12u;
inline constexpr uint32_t commit_lo=13u;
inline constexpr uint32_t commit_hi=14u;
inline constexpr uint32_t nonce_lo=15u;
inline constexpr uint32_t nonce_hi=16u;
inline constexpr uint32_t rawSHA_0=17u;
inline constexpr uint32_t rawSHA_1=18u;
inline constexpr uint32_t rawSHA_2=19u;
inline constexpr uint32_t rawSHA_3=20u;
inline constexpr uint32_t rawSHA_4=21u;
inline constexpr uint32_t rawSHA_5=22u;
inline constexpr uint32_t rawSHA_6=23u;
inline constexpr uint32_t rawSHA_7=24u;
inline constexpr uint32_t unitKind=25u;
inline constexpr uint32_t fieldLayoutRevision=26u;
inline constexpr uint32_t reserved0_27=27u;
inline constexpr uint32_t reserved0_28=28u;
inline constexpr uint32_t reserved0_29=29u;
inline constexpr uint32_t reserved0_30=30u;
inline constexpr uint32_t seal=31u;
}
namespace ColdHeader256 {
inline constexpr uint32_t magic=0u;
inline constexpr uint32_t schema=1u;
inline constexpr uint32_t profile=2u;
inline constexpr uint32_t headerWords=3u;
inline constexpr uint32_t mode=4u;
inline constexpr uint32_t flags=5u;
inline constexpr uint32_t segmentCount=6u;
inline constexpr uint32_t pageCount=7u;
inline constexpr uint32_t traceId_lo=8u;
inline constexpr uint32_t traceId_hi=9u;
inline constexpr uint32_t requestId_lo=10u;
inline constexpr uint32_t requestId_hi=11u;
inline constexpr uint32_t createdNativeNs_lo=12u;
inline constexpr uint32_t createdNativeNs_hi=13u;
inline constexpr uint32_t run_lo=14u;
inline constexpr uint32_t run_hi=15u;
inline constexpr uint32_t context_lo=16u;
inline constexpr uint32_t context_hi=17u;
inline constexpr uint32_t surface_lo=18u;
inline constexpr uint32_t surface_hi=19u;
inline constexpr uint32_t geometry_lo=20u;
inline constexpr uint32_t geometry_hi=21u;
inline constexpr uint32_t commit_lo=22u;
inline constexpr uint32_t commit_hi=23u;
inline constexpr uint32_t nonce_lo=24u;
inline constexpr uint32_t nonce_hi=25u;
inline constexpr uint32_t rawBytes_lo=26u;
inline constexpr uint32_t rawBytes_hi=27u;
inline constexpr uint32_t reserved0_28=28u;
inline constexpr uint32_t reserved0_29=29u;
inline constexpr uint32_t reserved0_30=30u;
inline constexpr uint32_t reserved0_31=31u;
inline constexpr uint32_t algorithmSHA_0=32u;
inline constexpr uint32_t algorithmSHA_1=33u;
inline constexpr uint32_t algorithmSHA_2=34u;
inline constexpr uint32_t algorithmSHA_3=35u;
inline constexpr uint32_t algorithmSHA_4=36u;
inline constexpr uint32_t algorithmSHA_5=37u;
inline constexpr uint32_t algorithmSHA_6=38u;
inline constexpr uint32_t algorithmSHA_7=39u;
inline constexpr uint32_t layoutSHA_0=40u;
inline constexpr uint32_t layoutSHA_1=41u;
inline constexpr uint32_t layoutSHA_2=42u;
inline constexpr uint32_t layoutSHA_3=43u;
inline constexpr uint32_t layoutSHA_4=44u;
inline constexpr uint32_t layoutSHA_5=45u;
inline constexpr uint32_t layoutSHA_6=46u;
inline constexpr uint32_t layoutSHA_7=47u;
inline constexpr uint32_t gatesSHA_0=48u;
inline constexpr uint32_t gatesSHA_1=49u;
inline constexpr uint32_t gatesSHA_2=50u;
inline constexpr uint32_t gatesSHA_3=51u;
inline constexpr uint32_t gatesSHA_4=52u;
inline constexpr uint32_t gatesSHA_5=53u;
inline constexpr uint32_t gatesSHA_6=54u;
inline constexpr uint32_t gatesSHA_7=55u;
inline constexpr uint32_t mathSHA_0=56u;
inline constexpr uint32_t mathSHA_1=57u;
inline constexpr uint32_t mathSHA_2=58u;
inline constexpr uint32_t mathSHA_3=59u;
inline constexpr uint32_t mathSHA_4=60u;
inline constexpr uint32_t mathSHA_5=61u;
inline constexpr uint32_t mathSHA_6=62u;
inline constexpr uint32_t mathSHA_7=63u;
inline constexpr uint32_t configSHA_0=64u;
inline constexpr uint32_t configSHA_1=65u;
inline constexpr uint32_t configSHA_2=66u;
inline constexpr uint32_t configSHA_3=67u;
inline constexpr uint32_t configSHA_4=68u;
inline constexpr uint32_t configSHA_5=69u;
inline constexpr uint32_t configSHA_6=70u;
inline constexpr uint32_t configSHA_7=71u;
inline constexpr uint32_t seedSHA_0=72u;
inline constexpr uint32_t seedSHA_1=73u;
inline constexpr uint32_t seedSHA_2=74u;
inline constexpr uint32_t seedSHA_3=75u;
inline constexpr uint32_t seedSHA_4=76u;
inline constexpr uint32_t seedSHA_5=77u;
inline constexpr uint32_t seedSHA_6=78u;
inline constexpr uint32_t seedSHA_7=79u;
inline constexpr uint32_t inputSHA_0=80u;
inline constexpr uint32_t inputSHA_1=81u;
inline constexpr uint32_t inputSHA_2=82u;
inline constexpr uint32_t inputSHA_3=83u;
inline constexpr uint32_t inputSHA_4=84u;
inline constexpr uint32_t inputSHA_5=85u;
inline constexpr uint32_t inputSHA_6=86u;
inline constexpr uint32_t inputSHA_7=87u;
inline constexpr uint32_t fixtureSHA_0=88u;
inline constexpr uint32_t fixtureSHA_1=89u;
inline constexpr uint32_t fixtureSHA_2=90u;
inline constexpr uint32_t fixtureSHA_3=91u;
inline constexpr uint32_t fixtureSHA_4=92u;
inline constexpr uint32_t fixtureSHA_5=93u;
inline constexpr uint32_t fixtureSHA_6=94u;
inline constexpr uint32_t fixtureSHA_7=95u;
inline constexpr uint32_t Nx=96u;
inline constexpr uint32_t Ny=97u;
inline constexpr uint32_t boundary=98u;
inline constexpr uint32_t rootKind=99u;
inline constexpr uint32_t outcome=100u;
inline constexpr uint32_t firstError=101u;
inline constexpr uint32_t rootTailBytes=102u;
inline constexpr uint32_t diagnosticMode=103u;
inline constexpr uint32_t reserved0_104=104u;
inline constexpr uint32_t reserved0_105=105u;
inline constexpr uint32_t reserved0_106=106u;
inline constexpr uint32_t reserved0_107=107u;
inline constexpr uint32_t reserved0_108=108u;
inline constexpr uint32_t reserved0_109=109u;
inline constexpr uint32_t reserved0_110=110u;
inline constexpr uint32_t reserved0_111=111u;
inline constexpr uint32_t reserved0_112=112u;
inline constexpr uint32_t reserved0_113=113u;
inline constexpr uint32_t reserved0_114=114u;
inline constexpr uint32_t reserved0_115=115u;
inline constexpr uint32_t reserved0_116=116u;
inline constexpr uint32_t reserved0_117=117u;
inline constexpr uint32_t reserved0_118=118u;
inline constexpr uint32_t reserved0_119=119u;
inline constexpr uint32_t reserved0_120=120u;
inline constexpr uint32_t reserved0_121=121u;
inline constexpr uint32_t reserved0_122=122u;
inline constexpr uint32_t reserved0_123=123u;
inline constexpr uint32_t reserved0_124=124u;
inline constexpr uint32_t reserved0_125=125u;
inline constexpr uint32_t reserved0_126=126u;
inline constexpr uint32_t reserved0_127=127u;
inline constexpr uint32_t reserved0_128=128u;
inline constexpr uint32_t reserved0_129=129u;
inline constexpr uint32_t reserved0_130=130u;
inline constexpr uint32_t reserved0_131=131u;
inline constexpr uint32_t reserved0_132=132u;
inline constexpr uint32_t reserved0_133=133u;
inline constexpr uint32_t reserved0_134=134u;
inline constexpr uint32_t reserved0_135=135u;
inline constexpr uint32_t reserved0_136=136u;
inline constexpr uint32_t reserved0_137=137u;
inline constexpr uint32_t reserved0_138=138u;
inline constexpr uint32_t reserved0_139=139u;
inline constexpr uint32_t reserved0_140=140u;
inline constexpr uint32_t reserved0_141=141u;
inline constexpr uint32_t reserved0_142=142u;
inline constexpr uint32_t reserved0_143=143u;
inline constexpr uint32_t reserved0_144=144u;
inline constexpr uint32_t reserved0_145=145u;
inline constexpr uint32_t reserved0_146=146u;
inline constexpr uint32_t reserved0_147=147u;
inline constexpr uint32_t reserved0_148=148u;
inline constexpr uint32_t reserved0_149=149u;
inline constexpr uint32_t reserved0_150=150u;
inline constexpr uint32_t reserved0_151=151u;
inline constexpr uint32_t reserved0_152=152u;
inline constexpr uint32_t reserved0_153=153u;
inline constexpr uint32_t reserved0_154=154u;
inline constexpr uint32_t reserved0_155=155u;
inline constexpr uint32_t reserved0_156=156u;
inline constexpr uint32_t reserved0_157=157u;
inline constexpr uint32_t reserved0_158=158u;
inline constexpr uint32_t reserved0_159=159u;
inline constexpr uint32_t reserved0_160=160u;
inline constexpr uint32_t reserved0_161=161u;
inline constexpr uint32_t reserved0_162=162u;
inline constexpr uint32_t reserved0_163=163u;
inline constexpr uint32_t reserved0_164=164u;
inline constexpr uint32_t reserved0_165=165u;
inline constexpr uint32_t reserved0_166=166u;
inline constexpr uint32_t reserved0_167=167u;
inline constexpr uint32_t reserved0_168=168u;
inline constexpr uint32_t reserved0_169=169u;
inline constexpr uint32_t reserved0_170=170u;
inline constexpr uint32_t reserved0_171=171u;
inline constexpr uint32_t reserved0_172=172u;
inline constexpr uint32_t reserved0_173=173u;
inline constexpr uint32_t reserved0_174=174u;
inline constexpr uint32_t reserved0_175=175u;
inline constexpr uint32_t reserved0_176=176u;
inline constexpr uint32_t reserved0_177=177u;
inline constexpr uint32_t reserved0_178=178u;
inline constexpr uint32_t reserved0_179=179u;
inline constexpr uint32_t reserved0_180=180u;
inline constexpr uint32_t reserved0_181=181u;
inline constexpr uint32_t reserved0_182=182u;
inline constexpr uint32_t reserved0_183=183u;
inline constexpr uint32_t reserved0_184=184u;
inline constexpr uint32_t reserved0_185=185u;
inline constexpr uint32_t reserved0_186=186u;
inline constexpr uint32_t reserved0_187=187u;
inline constexpr uint32_t reserved0_188=188u;
inline constexpr uint32_t reserved0_189=189u;
inline constexpr uint32_t reserved0_190=190u;
inline constexpr uint32_t reserved0_191=191u;
inline constexpr uint32_t reserved0_192=192u;
inline constexpr uint32_t reserved0_193=193u;
inline constexpr uint32_t reserved0_194=194u;
inline constexpr uint32_t reserved0_195=195u;
inline constexpr uint32_t reserved0_196=196u;
inline constexpr uint32_t reserved0_197=197u;
inline constexpr uint32_t reserved0_198=198u;
inline constexpr uint32_t reserved0_199=199u;
inline constexpr uint32_t reserved0_200=200u;
inline constexpr uint32_t reserved0_201=201u;
inline constexpr uint32_t reserved0_202=202u;
inline constexpr uint32_t reserved0_203=203u;
inline constexpr uint32_t reserved0_204=204u;
inline constexpr uint32_t reserved0_205=205u;
inline constexpr uint32_t reserved0_206=206u;
inline constexpr uint32_t reserved0_207=207u;
inline constexpr uint32_t reserved0_208=208u;
inline constexpr uint32_t reserved0_209=209u;
inline constexpr uint32_t reserved0_210=210u;
inline constexpr uint32_t reserved0_211=211u;
inline constexpr uint32_t reserved0_212=212u;
inline constexpr uint32_t reserved0_213=213u;
inline constexpr uint32_t reserved0_214=214u;
inline constexpr uint32_t reserved0_215=215u;
inline constexpr uint32_t reserved0_216=216u;
inline constexpr uint32_t reserved0_217=217u;
inline constexpr uint32_t reserved0_218=218u;
inline constexpr uint32_t reserved0_219=219u;
inline constexpr uint32_t reserved0_220=220u;
inline constexpr uint32_t reserved0_221=221u;
inline constexpr uint32_t reserved0_222=222u;
inline constexpr uint32_t reserved0_223=223u;
inline constexpr uint32_t reserved0_224=224u;
inline constexpr uint32_t reserved0_225=225u;
inline constexpr uint32_t reserved0_226=226u;
inline constexpr uint32_t reserved0_227=227u;
inline constexpr uint32_t reserved0_228=228u;
inline constexpr uint32_t reserved0_229=229u;
inline constexpr uint32_t reserved0_230=230u;
inline constexpr uint32_t reserved0_231=231u;
inline constexpr uint32_t reserved0_232=232u;
inline constexpr uint32_t reserved0_233=233u;
inline constexpr uint32_t reserved0_234=234u;
inline constexpr uint32_t reserved0_235=235u;
inline constexpr uint32_t reserved0_236=236u;
inline constexpr uint32_t reserved0_237=237u;
inline constexpr uint32_t reserved0_238=238u;
inline constexpr uint32_t reserved0_239=239u;
inline constexpr uint32_t reserved0_240=240u;
inline constexpr uint32_t reserved0_241=241u;
inline constexpr uint32_t reserved0_242=242u;
inline constexpr uint32_t reserved0_243=243u;
inline constexpr uint32_t reserved0_244=244u;
inline constexpr uint32_t reserved0_245=245u;
inline constexpr uint32_t reserved0_246=246u;
inline constexpr uint32_t reserved0_247=247u;
inline constexpr uint32_t canonicalPrefixSHA_selfZeroed_0=248u;
inline constexpr uint32_t canonicalPrefixSHA_selfZeroed_1=249u;
inline constexpr uint32_t canonicalPrefixSHA_selfZeroed_2=250u;
inline constexpr uint32_t canonicalPrefixSHA_selfZeroed_3=251u;
inline constexpr uint32_t canonicalPrefixSHA_selfZeroed_4=252u;
inline constexpr uint32_t canonicalPrefixSHA_selfZeroed_5=253u;
inline constexpr uint32_t canonicalPrefixSHA_selfZeroed_6=254u;
inline constexpr uint32_t canonicalPrefixSHA_selfZeroed_7=255u;
}
namespace NativePermit128 {
inline constexpr uint32_t schema=0u;
inline constexpr uint32_t rootKind=1u;
inline constexpr uint32_t flags=2u;
inline constexpr uint32_t candidateBank=3u;
inline constexpr uint32_t replayBank=4u;
inline constexpr uint32_t renderPackBank=5u;
inline constexpr uint32_t sourceHead=6u;
inline constexpr uint32_t requiredAuditMask=7u;
inline constexpr uint32_t nonce_lo=8u;
inline constexpr uint32_t nonce_hi=9u;
inline constexpr uint32_t plan_lo=10u;
inline constexpr uint32_t plan_hi=11u;
inline constexpr uint32_t run_lo=12u;
inline constexpr uint32_t run_hi=13u;
inline constexpr uint32_t owner_lo=14u;
inline constexpr uint32_t owner_hi=15u;
inline constexpr uint32_t context_lo=16u;
inline constexpr uint32_t context_hi=17u;
inline constexpr uint32_t surface_lo=18u;
inline constexpr uint32_t surface_hi=19u;
inline constexpr uint32_t geometry_lo=20u;
inline constexpr uint32_t geometry_hi=21u;
inline constexpr uint32_t configGeneration_lo=22u;
inline constexpr uint32_t configGeneration_hi=23u;
inline constexpr uint32_t pageSession_lo=24u;
inline constexpr uint32_t pageSession_hi=25u;
inline constexpr uint32_t permission_lo=26u;
inline constexpr uint32_t permission_hi=27u;
inline constexpr uint32_t inputEpoch_lo=28u;
inline constexpr uint32_t inputEpoch_hi=29u;
inline constexpr uint32_t controlGeneration_lo=30u;
inline constexpr uint32_t controlGeneration_hi=31u;
inline constexpr uint32_t baseCommit_lo=32u;
inline constexpr uint32_t baseCommit_hi=33u;
inline constexpr uint32_t proposedCommit_lo=34u;
inline constexpr uint32_t proposedCommit_hi=35u;
inline constexpr uint32_t startedNativeNs_lo=36u;
inline constexpr uint32_t startedNativeNs_hi=37u;
inline constexpr uint32_t deadlineNativeNs_lo=38u;
inline constexpr uint32_t deadlineNativeNs_hi=39u;
inline constexpr uint32_t configSHA_0=40u;
inline constexpr uint32_t configSHA_1=41u;
inline constexpr uint32_t configSHA_2=42u;
inline constexpr uint32_t configSHA_3=43u;
inline constexpr uint32_t configSHA_4=44u;
inline constexpr uint32_t configSHA_5=45u;
inline constexpr uint32_t configSHA_6=46u;
inline constexpr uint32_t configSHA_7=47u;
inline constexpr uint32_t algorithmSHA_0=48u;
inline constexpr uint32_t algorithmSHA_1=49u;
inline constexpr uint32_t algorithmSHA_2=50u;
inline constexpr uint32_t algorithmSHA_3=51u;
inline constexpr uint32_t algorithmSHA_4=52u;
inline constexpr uint32_t algorithmSHA_5=53u;
inline constexpr uint32_t algorithmSHA_6=54u;
inline constexpr uint32_t algorithmSHA_7=55u;
inline constexpr uint32_t layoutSHA_0=56u;
inline constexpr uint32_t layoutSHA_1=57u;
inline constexpr uint32_t layoutSHA_2=58u;
inline constexpr uint32_t layoutSHA_3=59u;
inline constexpr uint32_t layoutSHA_4=60u;
inline constexpr uint32_t layoutSHA_5=61u;
inline constexpr uint32_t layoutSHA_6=62u;
inline constexpr uint32_t layoutSHA_7=63u;
inline constexpr uint32_t gatesSHA_0=64u;
inline constexpr uint32_t gatesSHA_1=65u;
inline constexpr uint32_t gatesSHA_2=66u;
inline constexpr uint32_t gatesSHA_3=67u;
inline constexpr uint32_t gatesSHA_4=68u;
inline constexpr uint32_t gatesSHA_5=69u;
inline constexpr uint32_t gatesSHA_6=70u;
inline constexpr uint32_t gatesSHA_7=71u;
inline constexpr uint32_t inputSHA_0=72u;
inline constexpr uint32_t inputSHA_1=73u;
inline constexpr uint32_t inputSHA_2=74u;
inline constexpr uint32_t inputSHA_3=75u;
inline constexpr uint32_t inputSHA_4=76u;
inline constexpr uint32_t inputSHA_5=77u;
inline constexpr uint32_t inputSHA_6=78u;
inline constexpr uint32_t inputSHA_7=79u;
inline constexpr uint32_t acceptedBaseIdentitySHA_0=80u;
inline constexpr uint32_t acceptedBaseIdentitySHA_1=81u;
inline constexpr uint32_t acceptedBaseIdentitySHA_2=82u;
inline constexpr uint32_t acceptedBaseIdentitySHA_3=83u;
inline constexpr uint32_t acceptedBaseIdentitySHA_4=84u;
inline constexpr uint32_t acceptedBaseIdentitySHA_5=85u;
inline constexpr uint32_t acceptedBaseIdentitySHA_6=86u;
inline constexpr uint32_t acceptedBaseIdentitySHA_7=87u;
inline constexpr uint32_t acceptedQuantum_lo=88u;
inline constexpr uint32_t acceptedQuantum_hi=89u;
inline constexpr uint32_t sourceSeq_lo=90u;
inline constexpr uint32_t sourceSeq_hi=91u;
inline constexpr uint32_t diagnosticMode=92u;
inline constexpr uint32_t forcedJ=93u;
inline constexpr uint32_t viewRebind=94u;
inline constexpr uint32_t precisionFlags=95u;
inline constexpr uint32_t reserved0_96=96u;
inline constexpr uint32_t reserved0_97=97u;
inline constexpr uint32_t reserved0_98=98u;
inline constexpr uint32_t reserved0_99=99u;
inline constexpr uint32_t reserved0_100=100u;
inline constexpr uint32_t reserved0_101=101u;
inline constexpr uint32_t reserved0_102=102u;
inline constexpr uint32_t reserved0_103=103u;
inline constexpr uint32_t reserved0_104=104u;
inline constexpr uint32_t reserved0_105=105u;
inline constexpr uint32_t reserved0_106=106u;
inline constexpr uint32_t reserved0_107=107u;
inline constexpr uint32_t reserved0_108=108u;
inline constexpr uint32_t reserved0_109=109u;
inline constexpr uint32_t reserved0_110=110u;
inline constexpr uint32_t reserved0_111=111u;
inline constexpr uint32_t reserved0_112=112u;
inline constexpr uint32_t reserved0_113=113u;
inline constexpr uint32_t reserved0_114=114u;
inline constexpr uint32_t reserved0_115=115u;
inline constexpr uint32_t reserved0_116=116u;
inline constexpr uint32_t reserved0_117=117u;
inline constexpr uint32_t reserved0_118=118u;
inline constexpr uint32_t reserved0_119=119u;
inline constexpr uint32_t reserved0_120=120u;
inline constexpr uint32_t reserved0_121=121u;
inline constexpr uint32_t reserved0_122=122u;
inline constexpr uint32_t reserved0_123=123u;
inline constexpr uint32_t reserved0_124=124u;
inline constexpr uint32_t reserved0_125=125u;
inline constexpr uint32_t reserved0_126=126u;
inline constexpr uint32_t seal=127u;
}
namespace ViewPolicy64 {
inline constexpr uint32_t magic=0u;
inline constexpr uint32_t schema=1u;
inline constexpr uint32_t lengthWords=2u;
inline constexpr uint32_t mode=3u;
inline constexpr uint32_t speedMinF32=4u;
inline constexpr uint32_t speedMaxF32=5u;
inline constexpr uint32_t piMinF32=6u;
inline constexpr uint32_t piMaxF32=7u;
inline constexpr uint32_t curlMinF32=8u;
inline constexpr uint32_t curlMaxF32=9u;
inline constexpr uint32_t totalPressureMinF32=10u;
inline constexpr uint32_t totalPressureMaxF32=11u;
inline constexpr uint32_t tracerRadiusF32=12u;
inline constexpr uint32_t arrowScaleF32=13u;
inline constexpr uint32_t lineWidthF32=14u;
inline constexpr uint32_t flags=15u;
inline constexpr uint32_t configSHA_0=16u;
inline constexpr uint32_t configSHA_1=17u;
inline constexpr uint32_t configSHA_2=18u;
inline constexpr uint32_t configSHA_3=19u;
inline constexpr uint32_t configSHA_4=20u;
inline constexpr uint32_t configSHA_5=21u;
inline constexpr uint32_t configSHA_6=22u;
inline constexpr uint32_t configSHA_7=23u;
inline constexpr uint32_t viewSHA_selfZeroed_0=24u;
inline constexpr uint32_t viewSHA_selfZeroed_1=25u;
inline constexpr uint32_t viewSHA_selfZeroed_2=26u;
inline constexpr uint32_t viewSHA_selfZeroed_3=27u;
inline constexpr uint32_t viewSHA_selfZeroed_4=28u;
inline constexpr uint32_t viewSHA_selfZeroed_5=29u;
inline constexpr uint32_t viewSHA_selfZeroed_6=30u;
inline constexpr uint32_t viewSHA_selfZeroed_7=31u;
inline constexpr uint32_t layoutSHA_0=32u;
inline constexpr uint32_t layoutSHA_1=33u;
inline constexpr uint32_t layoutSHA_2=34u;
inline constexpr uint32_t layoutSHA_3=35u;
inline constexpr uint32_t layoutSHA_4=36u;
inline constexpr uint32_t layoutSHA_5=37u;
inline constexpr uint32_t layoutSHA_6=38u;
inline constexpr uint32_t layoutSHA_7=39u;
inline constexpr uint32_t reserved0_40=40u;
inline constexpr uint32_t reserved0_41=41u;
inline constexpr uint32_t reserved0_42=42u;
inline constexpr uint32_t reserved0_43=43u;
inline constexpr uint32_t reserved0_44=44u;
inline constexpr uint32_t reserved0_45=45u;
inline constexpr uint32_t reserved0_46=46u;
inline constexpr uint32_t reserved0_47=47u;
inline constexpr uint32_t reserved0_48=48u;
inline constexpr uint32_t reserved0_49=49u;
inline constexpr uint32_t reserved0_50=50u;
inline constexpr uint32_t reserved0_51=51u;
inline constexpr uint32_t reserved0_52=52u;
inline constexpr uint32_t reserved0_53=53u;
inline constexpr uint32_t reserved0_54=54u;
inline constexpr uint32_t reserved0_55=55u;
inline constexpr uint32_t reserved0_56=56u;
inline constexpr uint32_t reserved0_57=57u;
inline constexpr uint32_t reserved0_58=58u;
inline constexpr uint32_t reserved0_59=59u;
inline constexpr uint32_t reserved0_60=60u;
inline constexpr uint32_t reserved0_61=61u;
inline constexpr uint32_t reserved0_62=62u;
inline constexpr uint32_t seal=63u;
}
namespace MathQualification128 {
inline constexpr uint32_t schema=0u;
inline constexpr uint32_t backend=1u;
inline constexpr uint32_t qualificationFlags=2u;
inline constexpr uint32_t sampleCount=3u;
inline constexpr uint32_t mathSHA_0=4u;
inline constexpr uint32_t mathSHA_1=5u;
inline constexpr uint32_t mathSHA_2=6u;
inline constexpr uint32_t mathSHA_3=7u;
inline constexpr uint32_t mathSHA_4=8u;
inline constexpr uint32_t mathSHA_5=9u;
inline constexpr uint32_t mathSHA_6=10u;
inline constexpr uint32_t mathSHA_7=11u;
inline constexpr uint32_t programSHA_0=12u;
inline constexpr uint32_t programSHA_1=13u;
inline constexpr uint32_t programSHA_2=14u;
inline constexpr uint32_t programSHA_3=15u;
inline constexpr uint32_t programSHA_4=16u;
inline constexpr uint32_t programSHA_5=17u;
inline constexpr uint32_t programSHA_6=18u;
inline constexpr uint32_t programSHA_7=19u;
inline constexpr uint32_t deviceBuildSHA_0=20u;
inline constexpr uint32_t deviceBuildSHA_1=21u;
inline constexpr uint32_t deviceBuildSHA_2=22u;
inline constexpr uint32_t deviceBuildSHA_3=23u;
inline constexpr uint32_t deviceBuildSHA_4=24u;
inline constexpr uint32_t deviceBuildSHA_5=25u;
inline constexpr uint32_t deviceBuildSHA_6=26u;
inline constexpr uint32_t deviceBuildSHA_7=27u;
inline constexpr uint32_t reserved0_28=28u;
inline constexpr uint32_t reserved0_29=29u;
inline constexpr uint32_t reserved0_30=30u;
inline constexpr uint32_t reserved0_31=31u;
inline constexpr uint32_t sample0_functionId=32u;
inline constexpr uint32_t sample0_argumentBinary64Lo=33u;
inline constexpr uint32_t sample0_argumentBinary64Hi=34u;
inline constexpr uint32_t sample0_resultF32=35u;
inline constexpr uint32_t sample0_referenceF32=36u;
inline constexpr uint32_t sample0_errorBoundF32=37u;
inline constexpr uint32_t sample1_functionId=38u;
inline constexpr uint32_t sample1_argumentBinary64Lo=39u;
inline constexpr uint32_t sample1_argumentBinary64Hi=40u;
inline constexpr uint32_t sample1_resultF32=41u;
inline constexpr uint32_t sample1_referenceF32=42u;
inline constexpr uint32_t sample1_errorBoundF32=43u;
inline constexpr uint32_t sample2_functionId=44u;
inline constexpr uint32_t sample2_argumentBinary64Lo=45u;
inline constexpr uint32_t sample2_argumentBinary64Hi=46u;
inline constexpr uint32_t sample2_resultF32=47u;
inline constexpr uint32_t sample2_referenceF32=48u;
inline constexpr uint32_t sample2_errorBoundF32=49u;
inline constexpr uint32_t sample3_functionId=50u;
inline constexpr uint32_t sample3_argumentBinary64Lo=51u;
inline constexpr uint32_t sample3_argumentBinary64Hi=52u;
inline constexpr uint32_t sample3_resultF32=53u;
inline constexpr uint32_t sample3_referenceF32=54u;
inline constexpr uint32_t sample3_errorBoundF32=55u;
inline constexpr uint32_t sample4_functionId=56u;
inline constexpr uint32_t sample4_argumentBinary64Lo=57u;
inline constexpr uint32_t sample4_argumentBinary64Hi=58u;
inline constexpr uint32_t sample4_resultF32=59u;
inline constexpr uint32_t sample4_referenceF32=60u;
inline constexpr uint32_t sample4_errorBoundF32=61u;
inline constexpr uint32_t sample5_functionId=62u;
inline constexpr uint32_t sample5_argumentBinary64Lo=63u;
inline constexpr uint32_t sample5_argumentBinary64Hi=64u;
inline constexpr uint32_t sample5_resultF32=65u;
inline constexpr uint32_t sample5_referenceF32=66u;
inline constexpr uint32_t sample5_errorBoundF32=67u;
inline constexpr uint32_t sample6_functionId=68u;
inline constexpr uint32_t sample6_argumentBinary64Lo=69u;
inline constexpr uint32_t sample6_argumentBinary64Hi=70u;
inline constexpr uint32_t sample6_resultF32=71u;
inline constexpr uint32_t sample6_referenceF32=72u;
inline constexpr uint32_t sample6_errorBoundF32=73u;
inline constexpr uint32_t sample7_functionId=74u;
inline constexpr uint32_t sample7_argumentBinary64Lo=75u;
inline constexpr uint32_t sample7_argumentBinary64Hi=76u;
inline constexpr uint32_t sample7_resultF32=77u;
inline constexpr uint32_t sample7_referenceF32=78u;
inline constexpr uint32_t sample7_errorBoundF32=79u;
inline constexpr uint32_t sample8_functionId=80u;
inline constexpr uint32_t sample8_argumentBinary64Lo=81u;
inline constexpr uint32_t sample8_argumentBinary64Hi=82u;
inline constexpr uint32_t sample8_resultF32=83u;
inline constexpr uint32_t sample8_referenceF32=84u;
inline constexpr uint32_t sample8_errorBoundF32=85u;
inline constexpr uint32_t sample9_functionId=86u;
inline constexpr uint32_t sample9_argumentBinary64Lo=87u;
inline constexpr uint32_t sample9_argumentBinary64Hi=88u;
inline constexpr uint32_t sample9_resultF32=89u;
inline constexpr uint32_t sample9_referenceF32=90u;
inline constexpr uint32_t sample9_errorBoundF32=91u;
inline constexpr uint32_t sample10_functionId=92u;
inline constexpr uint32_t sample10_argumentBinary64Lo=93u;
inline constexpr uint32_t sample10_argumentBinary64Hi=94u;
inline constexpr uint32_t sample10_resultF32=95u;
inline constexpr uint32_t sample10_referenceF32=96u;
inline constexpr uint32_t sample10_errorBoundF32=97u;
inline constexpr uint32_t sample11_functionId=98u;
inline constexpr uint32_t sample11_argumentBinary64Lo=99u;
inline constexpr uint32_t sample11_argumentBinary64Hi=100u;
inline constexpr uint32_t sample11_resultF32=101u;
inline constexpr uint32_t sample11_referenceF32=102u;
inline constexpr uint32_t sample11_errorBoundF32=103u;
inline constexpr uint32_t sample12_functionId=104u;
inline constexpr uint32_t sample12_argumentBinary64Lo=105u;
inline constexpr uint32_t sample12_argumentBinary64Hi=106u;
inline constexpr uint32_t sample12_resultF32=107u;
inline constexpr uint32_t sample12_referenceF32=108u;
inline constexpr uint32_t sample12_errorBoundF32=109u;
inline constexpr uint32_t sample13_functionId=110u;
inline constexpr uint32_t sample13_argumentBinary64Lo=111u;
inline constexpr uint32_t sample13_argumentBinary64Hi=112u;
inline constexpr uint32_t sample13_resultF32=113u;
inline constexpr uint32_t sample13_referenceF32=114u;
inline constexpr uint32_t sample13_errorBoundF32=115u;
inline constexpr uint32_t sample14_functionId=116u;
inline constexpr uint32_t sample14_argumentBinary64Lo=117u;
inline constexpr uint32_t sample14_argumentBinary64Hi=118u;
inline constexpr uint32_t sample14_resultF32=119u;
inline constexpr uint32_t sample14_referenceF32=120u;
inline constexpr uint32_t sample14_errorBoundF32=121u;
inline constexpr uint32_t sample15_functionId=122u;
inline constexpr uint32_t sample15_argumentBinary64Lo=123u;
inline constexpr uint32_t sample15_argumentBinary64Hi=124u;
inline constexpr uint32_t sample15_resultF32=125u;
inline constexpr uint32_t sample15_referenceF32=126u;
inline constexpr uint32_t sample15_errorBoundF32=127u;
}
namespace QuotaAndClock64 {
inline constexpr uint32_t session_lo=0u;
inline constexpr uint32_t session_hi=1u;
inline constexpr uint32_t sequence_lo=2u;
inline constexpr uint32_t sequence_hi=3u;
inline constexpr uint32_t uiNs_lo=4u;
inline constexpr uint32_t uiNs_hi=5u;
inline constexpr uint32_t nativeReceiveNs_lo=6u;
inline constexpr uint32_t nativeReceiveNs_hi=7u;
inline constexpr uint32_t pulses_lo=8u;
inline constexpr uint32_t pulses_hi=9u;
inline constexpr uint32_t maxUiGapNs_lo=10u;
inline constexpr uint32_t maxUiGapNs_hi=11u;
inline constexpr uint32_t maxNativeGapNs_lo=12u;
inline constexpr uint32_t maxNativeGapNs_hi=13u;
inline constexpr uint32_t generated_lo=14u;
inline constexpr uint32_t generated_hi=15u;
inline constexpr uint32_t accepted_lo=16u;
inline constexpr uint32_t accepted_hi=17u;
inline constexpr uint32_t overload_lo=18u;
inline constexpr uint32_t overload_hi=19u;
inline constexpr uint32_t revoked_lo=20u;
inline constexpr uint32_t revoked_hi=21u;
inline constexpr uint32_t carry_lo=22u;
inline constexpr uint32_t carry_hi=23u;
inline constexpr uint32_t clearedCarry_lo=24u;
inline constexpr uint32_t clearedCarry_hi=25u;
inline constexpr uint32_t planSerial_lo=26u;
inline constexpr uint32_t planSerial_hi=27u;
inline constexpr uint32_t pending_lo=28u;
inline constexpr uint32_t pending_hi=29u;
inline constexpr uint32_t activeRemaining_lo=30u;
inline constexpr uint32_t activeRemaining_hi=31u;
inline constexpr uint32_t sourceBoundary_lo=32u;
inline constexpr uint32_t sourceBoundary_hi=33u;
inline constexpr uint32_t sourceIntent_lo=34u;
inline constexpr uint32_t sourceIntent_hi=35u;
inline constexpr uint32_t sourceControlGeneration_lo=36u;
inline constexpr uint32_t sourceControlGeneration_hi=37u;
inline constexpr uint32_t meaningfulInputSeq_lo=38u;
inline constexpr uint32_t meaningfulInputSeq_hi=39u;
inline constexpr uint32_t retiredPlan_lo=40u;
inline constexpr uint32_t retiredPlan_hi=41u;
inline constexpr uint32_t retiredNonce_lo=42u;
inline constexpr uint32_t retiredNonce_hi=43u;
inline constexpr uint32_t nativePollNs_lo=44u;
inline constexpr uint32_t nativePollNs_hi=45u;
inline constexpr uint32_t rootSubmitNs_lo=46u;
inline constexpr uint32_t rootSubmitNs_hi=47u;
inline constexpr uint32_t rootDeadlineNs_lo=48u;
inline constexpr uint32_t rootDeadlineNs_hi=49u;
inline constexpr uint32_t rootCompletionPollNs_lo=50u;
inline constexpr uint32_t rootCompletionPollNs_hi=51u;
inline constexpr uint32_t sceneNewQuantumFrames_lo=52u;
inline constexpr uint32_t sceneNewQuantumFrames_hi=53u;
inline constexpr uint32_t sceneSourceFrames_lo=54u;
inline constexpr uint32_t sceneSourceFrames_hi=55u;
inline constexpr uint32_t sceneRepeatFrames_lo=56u;
inline constexpr uint32_t sceneRepeatFrames_hi=57u;
inline constexpr uint32_t sceneStaleFrames_lo=58u;
inline constexpr uint32_t sceneStaleFrames_hi=59u;
inline constexpr uint32_t clockFault_lo=60u;
inline constexpr uint32_t clockFault_hi=61u;
inline constexpr uint32_t flags_lo=62u;
inline constexpr uint32_t flags_hi=63u;
}
namespace TaggedReceiptHeader16 {
inline constexpr uint32_t tag=0u;
inline constexpr uint32_t schema=1u;
inline constexpr uint32_t encoding=2u;
inline constexpr uint32_t wordLength=3u;
inline constexpr uint32_t usedBytes=4u;
inline constexpr uint32_t flags=5u;
inline constexpr uint32_t payloadSHA_0=6u;
inline constexpr uint32_t payloadSHA_1=7u;
inline constexpr uint32_t payloadSHA_2=8u;
inline constexpr uint32_t payloadSHA_3=9u;
inline constexpr uint32_t payloadSHA_4=10u;
inline constexpr uint32_t payloadSHA_5=11u;
inline constexpr uint32_t payloadSHA_6=12u;
inline constexpr uint32_t payloadSHA_7=13u;
inline constexpr uint32_t reserved0_14=14u;
inline constexpr uint32_t seal=15u;
}
namespace GpuGlobal256 {
inline constexpr uint32_t rootStatus=0u;
inline constexpr uint32_t firstErrorOp=1u;
inline constexpr uint32_t firstError=2u;
inline constexpr uint32_t scope=3u;
inline constexpr uint32_t trialJ=4u;
inline constexpr uint32_t micro=5u;
inline constexpr uint32_t selectedJActual=6u;
inline constexpr uint32_t selectedJReplay=7u;
inline constexpr uint32_t rootKind=8u;
inline constexpr uint32_t activeMask=9u;
inline constexpr uint32_t activePasses=10u;
inline constexpr uint32_t noopPasses=11u;
inline constexpr uint32_t pressureFrozen=12u;
inline constexpr uint32_t viscosityFrozen=13u;
inline constexpr uint32_t scopeFailed=14u;
inline constexpr uint32_t traceMicroIndex=15u;
inline constexpr uint32_t sourceSeq_lo=16u;
inline constexpr uint32_t sourceSeq_hi=17u;
inline constexpr uint32_t nonce_lo=18u;
inline constexpr uint32_t nonce_hi=19u;
inline constexpr uint32_t plan_lo=20u;
inline constexpr uint32_t plan_hi=21u;
inline constexpr uint32_t baseCommit_lo=22u;
inline constexpr uint32_t baseCommit_hi=23u;
inline constexpr uint32_t proposedCommit_lo=24u;
inline constexpr uint32_t proposedCommit_hi=25u;
inline constexpr uint32_t baseQuantum_lo=26u;
inline constexpr uint32_t baseQuantum_hi=27u;
inline constexpr uint32_t proposedQuantum_lo=28u;
inline constexpr uint32_t proposedQuantum_hi=29u;
inline constexpr uint32_t controlGeneration_lo=30u;
inline constexpr uint32_t controlGeneration_hi=31u;
inline constexpr uint32_t KwindowStart_valueF32=32u;
inline constexpr uint32_t KwindowStart_errorBoundF32=33u;
inline constexpr uint32_t K_valueF32=34u;
inline constexpr uint32_t K_errorBoundF32=35u;
inline constexpr uint32_t WEuler_valueF32=36u;
inline constexpr uint32_t WEuler_errorBoundF32=37u;
inline constexpr uint32_t Wnonpotential_valueF32=38u;
inline constexpr uint32_t Wnonpotential_errorBoundF32=39u;
inline constexpr uint32_t Wwall_valueF32=40u;
inline constexpr uint32_t Wwall_errorBoundF32=41u;
inline constexpr uint32_t Wpositive_valueF32=42u;
inline constexpr uint32_t Wpositive_errorBoundF32=43u;
inline constexpr uint32_t Dadv_valueF32=44u;
inline constexpr uint32_t Dadv_errorBoundF32=45u;
inline constexpr uint32_t Dnu_valueF32=46u;
inline constexpr uint32_t Dnu_errorBoundF32=47u;
inline constexpr uint32_t DBE_valueF32=48u;
inline constexpr uint32_t DBE_errorBoundF32=49u;
inline constexpr uint32_t Dprojection_valueF32=50u;
inline constexpr uint32_t Dprojection_errorBoundF32=51u;
inline constexpr uint32_t Rdualdiv_valueF32=52u;
inline constexpr uint32_t Rdualdiv_errorBoundF32=53u;
inline constexpr uint32_t RH_valueF32=54u;
inline constexpr uint32_t RH_errorBoundF32=55u;
inline constexpr uint32_t RP_valueF32=56u;
inline constexpr uint32_t RP_errorBoundF32=57u;
inline constexpr uint32_t RrhsQuantization_valueF32=58u;
inline constexpr uint32_t RrhsQuantization_errorBoundF32=59u;
inline constexpr uint32_t Rfloat_valueF32=60u;
inline constexpr uint32_t Rfloat_errorBoundF32=61u;
inline constexpr uint32_t windowAbsWork_valueF32=62u;
inline constexpr uint32_t windowAbsWork_errorBoundF32=63u;
inline constexpr uint32_t windowDissipation_valueF32=64u;
inline constexpr uint32_t windowDissipation_errorBoundF32=65u;
inline constexpr uint32_t Escalefrozen_valueF32=66u;
inline constexpr uint32_t Escalefrozen_errorBoundF32=67u;
inline constexpr uint32_t momentumClosureX_valueF32=68u;
inline constexpr uint32_t momentumClosureX_errorBoundF32=69u;
inline constexpr uint32_t momentumClosureY_valueF32=70u;
inline constexpr uint32_t momentumClosureY_errorBoundF32=71u;
inline constexpr uint32_t CFLincoming_valueF32=72u;
inline constexpr uint32_t CFLincoming_errorBoundF32=73u;
inline constexpr uint32_t CFLoutgoing_valueF32=74u;
inline constexpr uint32_t CFLoutgoing_errorBoundF32=75u;
inline constexpr uint32_t divergenceRMS_valueF32=76u;
inline constexpr uint32_t divergenceRMS_errorBoundF32=77u;
inline constexpr uint32_t divergenceMax_valueF32=78u;
inline constexpr uint32_t divergenceMax_errorBoundF32=79u;
inline constexpr uint32_t tickBefore_lo=80u;
inline constexpr uint32_t tickBefore_hi=81u;
inline constexpr uint32_t tickAfter_lo=82u;
inline constexpr uint32_t tickAfter_hi=83u;
inline constexpr uint32_t sourceIntent_lo=84u;
inline constexpr uint32_t sourceIntent_hi=85u;
inline constexpr uint32_t sourceBoundary_lo=86u;
inline constexpr uint32_t sourceBoundary_hi=87u;
inline constexpr uint32_t parameterGeneration_lo=88u;
inline constexpr uint32_t parameterGeneration_hi=89u;
inline constexpr uint32_t rootTraceGeneration_lo=90u;
inline constexpr uint32_t rootTraceGeneration_hi=91u;
inline constexpr uint32_t renderGeneration_lo=92u;
inline constexpr uint32_t renderGeneration_hi=93u;
inline constexpr uint32_t scopeNonce_lo=94u;
inline constexpr uint32_t scopeNonce_hi=95u;
inline constexpr uint32_t failedTrialNonce_lo=96u;
inline constexpr uint32_t failedTrialNonce_hi=97u;
inline constexpr uint32_t windowFirstQuantum_lo=98u;
inline constexpr uint32_t windowFirstQuantum_hi=99u;
inline constexpr uint32_t windowLastQuantum_lo=100u;
inline constexpr uint32_t windowLastQuantum_hi=101u;
inline constexpr uint32_t windowAcceptedQuanta_lo=102u;
inline constexpr uint32_t windowAcceptedQuanta_hi=103u;
inline constexpr uint32_t sumBPreQ32_lo=104u;
inline constexpr uint32_t sumBPreQ32_hi=105u;
inline constexpr uint32_t sumBPostQ32_lo=106u;
inline constexpr uint32_t sumBPostQ32_hi=107u;
inline constexpr uint32_t piMeanSumQ32_lo=108u;
inline constexpr uint32_t piMeanSumQ32_hi=109u;
inline constexpr uint32_t pressureMaxRawQ32_lo=110u;
inline constexpr uint32_t pressureMaxRawQ32_hi=111u;
inline constexpr uint32_t drive_0=112u;
inline constexpr uint32_t drive_1=113u;
inline constexpr uint32_t drive_2=114u;
inline constexpr uint32_t drive_3=115u;
inline constexpr uint32_t drive_4=116u;
inline constexpr uint32_t drive_5=117u;
inline constexpr uint32_t drive_6=118u;
inline constexpr uint32_t drive_7=119u;
inline constexpr uint32_t drive_8=120u;
inline constexpr uint32_t drive_9=121u;
inline constexpr uint32_t drive_10=122u;
inline constexpr uint32_t drive_11=123u;
inline constexpr uint32_t drive_12=124u;
inline constexpr uint32_t drive_13=125u;
inline constexpr uint32_t drive_14=126u;
inline constexpr uint32_t drive_15=127u;
inline constexpr uint32_t drive_16=128u;
inline constexpr uint32_t drive_17=129u;
inline constexpr uint32_t drive_18=130u;
inline constexpr uint32_t drive_19=131u;
inline constexpr uint32_t drive_20=132u;
inline constexpr uint32_t drive_21=133u;
inline constexpr uint32_t drive_22=134u;
inline constexpr uint32_t drive_23=135u;
inline constexpr uint32_t drive_24=136u;
inline constexpr uint32_t drive_25=137u;
inline constexpr uint32_t drive_26=138u;
inline constexpr uint32_t drive_27=139u;
inline constexpr uint32_t drive_28=140u;
inline constexpr uint32_t drive_29=141u;
inline constexpr uint32_t drive_30=142u;
inline constexpr uint32_t drive_31=143u;
inline constexpr uint32_t proof_0=144u;
inline constexpr uint32_t proof_1=145u;
inline constexpr uint32_t proof_2=146u;
inline constexpr uint32_t proof_3=147u;
inline constexpr uint32_t proof_4=148u;
inline constexpr uint32_t proof_5=149u;
inline constexpr uint32_t proof_6=150u;
inline constexpr uint32_t proof_7=151u;
inline constexpr uint32_t proof_8=152u;
inline constexpr uint32_t proof_9=153u;
inline constexpr uint32_t proof_10=154u;
inline constexpr uint32_t proof_11=155u;
inline constexpr uint32_t proof_12=156u;
inline constexpr uint32_t proof_13=157u;
inline constexpr uint32_t proof_14=158u;
inline constexpr uint32_t proof_15=159u;
inline constexpr uint32_t proof_16=160u;
inline constexpr uint32_t proof_17=161u;
inline constexpr uint32_t proof_18=162u;
inline constexpr uint32_t proof_19=163u;
inline constexpr uint32_t proof_20=164u;
inline constexpr uint32_t proof_21=165u;
inline constexpr uint32_t proof_22=166u;
inline constexpr uint32_t proof_23=167u;
inline constexpr uint32_t proof_24=168u;
inline constexpr uint32_t proof_25=169u;
inline constexpr uint32_t proof_26=170u;
inline constexpr uint32_t proof_27=171u;
inline constexpr uint32_t proof_28=172u;
inline constexpr uint32_t proof_29=173u;
inline constexpr uint32_t proof_30=174u;
inline constexpr uint32_t proof_31=175u;
inline constexpr uint32_t momentumAndWalls_0=176u;
inline constexpr uint32_t momentumAndWalls_1=177u;
inline constexpr uint32_t momentumAndWalls_2=178u;
inline constexpr uint32_t momentumAndWalls_3=179u;
inline constexpr uint32_t momentumAndWalls_4=180u;
inline constexpr uint32_t momentumAndWalls_5=181u;
inline constexpr uint32_t momentumAndWalls_6=182u;
inline constexpr uint32_t momentumAndWalls_7=183u;
inline constexpr uint32_t momentumAndWalls_8=184u;
inline constexpr uint32_t momentumAndWalls_9=185u;
inline constexpr uint32_t momentumAndWalls_10=186u;
inline constexpr uint32_t momentumAndWalls_11=187u;
inline constexpr uint32_t momentumAndWalls_12=188u;
inline constexpr uint32_t momentumAndWalls_13=189u;
inline constexpr uint32_t momentumAndWalls_14=190u;
inline constexpr uint32_t momentumAndWalls_15=191u;
inline constexpr uint32_t momentumAndWalls_16=192u;
inline constexpr uint32_t momentumAndWalls_17=193u;
inline constexpr uint32_t momentumAndWalls_18=194u;
inline constexpr uint32_t momentumAndWalls_19=195u;
inline constexpr uint32_t momentumAndWalls_20=196u;
inline constexpr uint32_t momentumAndWalls_21=197u;
inline constexpr uint32_t momentumAndWalls_22=198u;
inline constexpr uint32_t momentumAndWalls_23=199u;
inline constexpr uint32_t momentumAndWalls_24=200u;
inline constexpr uint32_t momentumAndWalls_25=201u;
inline constexpr uint32_t momentumAndWalls_26=202u;
inline constexpr uint32_t momentumAndWalls_27=203u;
inline constexpr uint32_t momentumAndWalls_28=204u;
inline constexpr uint32_t momentumAndWalls_29=205u;
inline constexpr uint32_t momentumAndWalls_30=206u;
inline constexpr uint32_t momentumAndWalls_31=207u;
inline constexpr uint32_t conversionAndProbe_0=208u;
inline constexpr uint32_t conversionAndProbe_1=209u;
inline constexpr uint32_t conversionAndProbe_2=210u;
inline constexpr uint32_t conversionAndProbe_3=211u;
inline constexpr uint32_t conversionAndProbe_4=212u;
inline constexpr uint32_t conversionAndProbe_5=213u;
inline constexpr uint32_t conversionAndProbe_6=214u;
inline constexpr uint32_t conversionAndProbe_7=215u;
inline constexpr uint32_t conversionAndProbe_8=216u;
inline constexpr uint32_t conversionAndProbe_9=217u;
inline constexpr uint32_t conversionAndProbe_10=218u;
inline constexpr uint32_t conversionAndProbe_11=219u;
inline constexpr uint32_t conversionAndProbe_12=220u;
inline constexpr uint32_t conversionAndProbe_13=221u;
inline constexpr uint32_t conversionAndProbe_14=222u;
inline constexpr uint32_t conversionAndProbe_15=223u;
inline constexpr uint32_t conversionAndProbe_16=224u;
inline constexpr uint32_t conversionAndProbe_17=225u;
inline constexpr uint32_t conversionAndProbe_18=226u;
inline constexpr uint32_t conversionAndProbe_19=227u;
inline constexpr uint32_t conversionAndProbe_20=228u;
inline constexpr uint32_t conversionAndProbe_21=229u;
inline constexpr uint32_t conversionAndProbe_22=230u;
inline constexpr uint32_t conversionAndProbe_23=231u;
inline constexpr uint32_t conversionAndProbe_24=232u;
inline constexpr uint32_t conversionAndProbe_25=233u;
inline constexpr uint32_t conversionAndProbe_26=234u;
inline constexpr uint32_t conversionAndProbe_27=235u;
inline constexpr uint32_t conversionAndProbe_28=236u;
inline constexpr uint32_t conversionAndProbe_29=237u;
inline constexpr uint32_t conversionAndProbe_30=238u;
inline constexpr uint32_t conversionAndProbe_31=239u;
inline constexpr uint32_t scheduleIndex=240u;
inline constexpr uint32_t requiredPasses=241u;
inline constexpr uint32_t requiredAuditMask=242u;
inline constexpr uint32_t actualAuditMask=243u;
inline constexpr uint32_t traceSealedMask=244u;
inline constexpr uint32_t rootComplete=245u;
inline constexpr uint32_t candidateBank=246u;
inline constexpr uint32_t replayBank=247u;
inline constexpr uint32_t renderPackBank=248u;
inline constexpr uint32_t viewRebind=249u;
inline constexpr uint32_t diagnosticMode=250u;
inline constexpr uint32_t forcedJ=251u;
inline constexpr uint32_t precisionPolicy=252u;
inline constexpr uint32_t mathBackend=253u;
inline constexpr uint32_t flags=254u;
inline constexpr uint32_t seal=255u;
}
inline constexpr uint32_t OP_ROOT_INIT=0u;
inline constexpr uint32_t OP_ROOT_INPUT_GATE=1u;
inline constexpr uint32_t OP_ROOT_BASE_AUDIT=2u;
inline constexpr uint32_t OP_ROOT_WINDOW_SNAPSHOT=3u;
inline constexpr uint32_t OP_ROOT_ACTUAL_BEGIN=4u;
inline constexpr uint32_t OP_ROOT_REPLAY_BEGIN=5u;
inline constexpr uint32_t OP_ROOT_SCOPE_COMPARE=6u;
inline constexpr uint32_t OP_ROOT_FINAL_FIELD_AUDIT=7u;
inline constexpr uint32_t OP_ROOT_TIME_PREPARE=8u;
inline constexpr uint32_t OP_PACK_VIEW=9u;
inline constexpr uint32_t OP_PACK_AUDIT=10u;
inline constexpr uint32_t OP_ROOT_COMPLETION_SEAL=11u;
inline constexpr uint32_t OP_TRIAL_GATE=12u;
inline constexpr uint32_t OP_TRIAL_RESET_FROM_BASE=13u;
inline constexpr uint32_t OP_TRIAL_INIT=14u;
inline constexpr uint32_t OP_TRIAL_ENTRY_AUDIT=15u;
inline constexpr uint32_t OP_ENTRY_PARTIAL=16u;
inline constexpr uint32_t OP_ENTRY_FINAL=17u;
inline constexpr uint32_t OP_ADVECT=18u;
inline constexpr uint32_t OP_ADVECT_ENERGY_PARTIAL=19u;
inline constexpr uint32_t OP_ADVECT_ENERGY_FINAL=20u;
inline constexpr uint32_t OP_BODY_KICK=21u;
inline constexpr uint32_t OP_BODY_WORK_REDUCE=22u;
inline constexpr uint32_t OP_CORIOLIS_0_F=23u;
inline constexpr uint32_t OP_CORIOLIS_1_F=24u;
inline constexpr uint32_t OP_CORIOLIS_2_F=25u;
inline constexpr uint32_t OP_CORIOLIS_3_F=26u;
inline constexpr uint32_t OP_CORIOLIS_3_R=27u;
inline constexpr uint32_t OP_CORIOLIS_2_R=28u;
inline constexpr uint32_t OP_CORIOLIS_1_R=29u;
inline constexpr uint32_t OP_CORIOLIS_0_R=30u;
inline constexpr uint32_t OP_CORIOLIS_ENERGY_PARTIAL=31u;
inline constexpr uint32_t OP_CORIOLIS_ENERGY_FINAL=32u;
inline constexpr uint32_t OP_VISC_SETUP=33u;
inline constexpr uint32_t OP_VISC_ENERGY_PARTIAL=34u;
inline constexpr uint32_t OP_VISC_ENERGY_FINAL=35u;
inline constexpr uint32_t OP_PRESSURE_RHS=36u;
inline constexpr uint32_t OP_PRESSURE_RHS_COMPAT=37u;
inline constexpr uint32_t OP_PRESSURE_INIT=38u;
inline constexpr uint32_t OP_PROJECT=39u;
inline constexpr uint32_t OP_ENFORCE_NORMAL_FACES=40u;
inline constexpr uint32_t OP_FLOW_AUDIT_PARTIAL=41u;
inline constexpr uint32_t OP_FLOW_AUDIT_FINAL=42u;
inline constexpr uint32_t OP_TRACER_ADVECT=43u;
inline constexpr uint32_t OP_TRACER_CHECK=44u;
inline constexpr uint32_t OP_ENERGY_LEDGER_PARTIAL=45u;
inline constexpr uint32_t OP_ENERGY_LEDGER_FINAL=46u;
inline constexpr uint32_t OP_MICRO_SEAL=47u;
inline constexpr uint32_t OP_VISC_BLOCK=48u;
inline constexpr uint32_t OP_PRESSURE_V=49u;
inline constexpr uint32_t OP_SOURCE_INIT=50u;
inline constexpr uint32_t OP_SOURCE_INPUT_GATE=51u;
inline constexpr uint32_t OP_SOURCE_BASE_AUDIT=52u;
inline constexpr uint32_t OP_SOURCE_DEDUP_GATE=53u;
inline constexpr uint32_t OP_SOURCE_ACTUAL_COPY=54u;
inline constexpr uint32_t OP_SOURCE_ACTUAL_INSTALL=55u;
inline constexpr uint32_t OP_SOURCE_ACTUAL_AUDIT=56u;
inline constexpr uint32_t OP_SOURCE_REPLAY_COPY=57u;
inline constexpr uint32_t OP_SOURCE_REPLAY_INSTALL=58u;
inline constexpr uint32_t OP_SOURCE_REPLAY_AUDIT=59u;
inline constexpr uint32_t OP_SOURCE_COMPARE=60u;
inline constexpr uint32_t OP_SOURCE_CONTROL_PREPARE=61u;
inline constexpr uint32_t OP_SOURCE_TIME_ZERO_AUDIT=62u;
inline constexpr uint32_t OP_SOURCE_COMPLETION_SEAL=63u;
inline constexpr Pass k_quantum[]={
{0u,0u,0u,0u,0u},
{1u,0u,0u,0u,0u},
{2u,0u,0u,0u,0u},
{3u,0u,0u,0u,0u},
{4u,0u,0u,0u,0u},
{12u,1u,1u,0u,0u},
{13u,1u,1u,0u,0u},
{14u,1u,1u,0u,0u},
{15u,1u,1u,0u,0u},
{16u,1u,1u,0u,0u},
{17u,1u,1u,0u,0u},
{18u,1u,1u,0u,0u},
{19u,1u,1u,0u,0u},
{20u,1u,1u,0u,0u},
{21u,1u,1u,0u,0u},
{22u,1u,1u,0u,0u},
{23u,1u,1u,0u,0u},
{24u,1u,1u,0u,0u},
{25u,1u,1u,0u,0u},
{26u,1u,1u,0u,0u},
{27u,1u,1u,0u,0u},
{28u,1u,1u,0u,0u},
{29u,1u,1u,0u,0u},
{30u,1u,1u,0u,0u},
{31u,1u,1u,0u,0u},
{32u,1u,1u,0u,0u},
{33u,1u,1u,0u,0u},
{48u,1u,1u,0u,0u},
{48u,1u,1u,0u,1u},
{48u,1u,1u,0u,2u},
{48u,1u,1u,0u,3u},
{48u,1u,1u,0u,4u},
{48u,1u,1u,0u,5u},
{48u,1u,1u,0u,6u},
{48u,1u,1u,0u,7u},
{34u,1u,1u,0u,0u},
{35u,1u,1u,0u,0u},
{36u,1u,1u,0u,0u},
{37u,1u,1u,0u,0u},
{38u,1u,1u,0u,0u},
{49u,1u,1u,0u,0u},
{49u,1u,1u,0u,1u},
{49u,1u,1u,0u,2u},
{49u,1u,1u,0u,3u},
{49u,1u,1u,0u,4u},
{49u,1u,1u,0u,5u},
{49u,1u,1u,0u,6u},
{49u,1u,1u,0u,7u},
{49u,1u,1u,0u,8u},
{49u,1u,1u,0u,9u},
{49u,1u,1u,0u,10u},
{49u,1u,1u,0u,11u},
{49u,1u,1u,0u,12u},
{49u,1u,1u,0u,13u},
{49u,1u,1u,0u,14u},
{49u,1u,1u,0u,15u},
{39u,1u,1u,0u,0u},
{40u,1u,1u,0u,0u},
{41u,1u,1u,0u,0u},
{42u,1u,1u,0u,0u},
{43u,1u,1u,0u,0u},
{44u,1u,1u,0u,0u},
{45u,1u,1u,0u,0u},
{46u,1u,1u,0u,0u},
{47u,1u,1u,0u,0u},
{12u,1u,2u,0u,0u},
{13u,1u,2u,0u,0u},
{14u,1u,2u,0u,0u},
{15u,1u,2u,0u,0u},
{16u,1u,2u,0u,0u},
{17u,1u,2u,0u,0u},
{18u,1u,2u,0u,0u},
{19u,1u,2u,0u,0u},
{20u,1u,2u,0u,0u},
{21u,1u,2u,0u,0u},
{22u,1u,2u,0u,0u},
{23u,1u,2u,0u,0u},
{24u,1u,2u,0u,0u},
{25u,1u,2u,0u,0u},
{26u,1u,2u,0u,0u},
{27u,1u,2u,0u,0u},
{28u,1u,2u,0u,0u},
{29u,1u,2u,0u,0u},
{30u,1u,2u,0u,0u},
{31u,1u,2u,0u,0u},
{32u,1u,2u,0u,0u},
{33u,1u,2u,0u,0u},
{48u,1u,2u,0u,0u},
{48u,1u,2u,0u,1u},
{48u,1u,2u,0u,2u},
{48u,1u,2u,0u,3u},
{48u,1u,2u,0u,4u},
{48u,1u,2u,0u,5u},
{48u,1u,2u,0u,6u},
{48u,1u,2u,0u,7u},
{34u,1u,2u,0u,0u},
{35u,1u,2u,0u,0u},
{36u,1u,2u,0u,0u},
{37u,1u,2u,0u,0u},
{38u,1u,2u,0u,0u},
{49u,1u,2u,0u,0u},
{49u,1u,2u,0u,1u},
{49u,1u,2u,0u,2u},
{49u,1u,2u,0u,3u},
{49u,1u,2u,0u,4u},
{49u,1u,2u,0u,5u},
{49u,1u,2u,0u,6u},
{49u,1u,2u,0u,7u},
{49u,1u,2u,0u,8u},
{49u,1u,2u,0u,9u},
{49u,1u,2u,0u,10u},
{49u,1u,2u,0u,11u},
{49u,1u,2u,0u,12u},
{49u,1u,2u,0u,13u},
{49u,1u,2u,0u,14u},
{49u,1u,2u,0u,15u},
{39u,1u,2u,0u,0u},
{40u,1u,2u,0u,0u},
{41u,1u,2u,0u,0u},
{42u,1u,2u,0u,0u},
{43u,1u,2u,0u,0u},
{44u,1u,2u,0u,0u},
{45u,1u,2u,0u,0u},
{46u,1u,2u,0u,0u},
{47u,1u,2u,0u,0u},
{16u,1u,2u,1u,0u},
{17u,1u,2u,1u,0u},
{18u,1u,2u,1u,0u},
{19u,1u,2u,1u,0u},
{20u,1u,2u,1u,0u},
{21u,1u,2u,1u,0u},
{22u,1u,2u,1u,0u},
{23u,1u,2u,1u,0u},
{24u,1u,2u,1u,0u},
{25u,1u,2u,1u,0u},
{26u,1u,2u,1u,0u},
{27u,1u,2u,1u,0u},
{28u,1u,2u,1u,0u},
{29u,1u,2u,1u,0u},
{30u,1u,2u,1u,0u},
{31u,1u,2u,1u,0u},
{32u,1u,2u,1u,0u},
{33u,1u,2u,1u,0u},
{48u,1u,2u,1u,0u},
{48u,1u,2u,1u,1u},
{48u,1u,2u,1u,2u},
{48u,1u,2u,1u,3u},
{48u,1u,2u,1u,4u},
{48u,1u,2u,1u,5u},
{48u,1u,2u,1u,6u},
{48u,1u,2u,1u,7u},
{34u,1u,2u,1u,0u},
{35u,1u,2u,1u,0u},
{36u,1u,2u,1u,0u},
{37u,1u,2u,1u,0u},
{38u,1u,2u,1u,0u},
{49u,1u,2u,1u,0u},
{49u,1u,2u,1u,1u},
{49u,1u,2u,1u,2u},
{49u,1u,2u,1u,3u},
{49u,1u,2u,1u,4u},
{49u,1u,2u,1u,5u},
{49u,1u,2u,1u,6u},
{49u,1u,2u,1u,7u},
{49u,1u,2u,1u,8u},
{49u,1u,2u,1u,9u},
{49u,1u,2u,1u,10u},
{49u,1u,2u,1u,11u},
{49u,1u,2u,1u,12u},
{49u,1u,2u,1u,13u},
{49u,1u,2u,1u,14u},
{49u,1u,2u,1u,15u},
{39u,1u,2u,1u,0u},
{40u,1u,2u,1u,0u},
{41u,1u,2u,1u,0u},
{42u,1u,2u,1u,0u},
{43u,1u,2u,1u,0u},
{44u,1u,2u,1u,0u},
{45u,1u,2u,1u,0u},
{46u,1u,2u,1u,0u},
{47u,1u,2u,1u,0u},
{12u,1u,4u,0u,0u},
{13u,1u,4u,0u,0u},
{14u,1u,4u,0u,0u},
{15u,1u,4u,0u,0u},
{16u,1u,4u,0u,0u},
{17u,1u,4u,0u,0u},
{18u,1u,4u,0u,0u},
{19u,1u,4u,0u,0u},
{20u,1u,4u,0u,0u},
{21u,1u,4u,0u,0u},
{22u,1u,4u,0u,0u},
{23u,1u,4u,0u,0u},
{24u,1u,4u,0u,0u},
{25u,1u,4u,0u,0u},
{26u,1u,4u,0u,0u},
{27u,1u,4u,0u,0u},
{28u,1u,4u,0u,0u},
{29u,1u,4u,0u,0u},
{30u,1u,4u,0u,0u},
{31u,1u,4u,0u,0u},
{32u,1u,4u,0u,0u},
{33u,1u,4u,0u,0u},
{48u,1u,4u,0u,0u},
{48u,1u,4u,0u,1u},
{48u,1u,4u,0u,2u},
{48u,1u,4u,0u,3u},
{48u,1u,4u,0u,4u},
{48u,1u,4u,0u,5u},
{48u,1u,4u,0u,6u},
{48u,1u,4u,0u,7u},
{34u,1u,4u,0u,0u},
{35u,1u,4u,0u,0u},
{36u,1u,4u,0u,0u},
{37u,1u,4u,0u,0u},
{38u,1u,4u,0u,0u},
{49u,1u,4u,0u,0u},
{49u,1u,4u,0u,1u},
{49u,1u,4u,0u,2u},
{49u,1u,4u,0u,3u},
{49u,1u,4u,0u,4u},
{49u,1u,4u,0u,5u},
{49u,1u,4u,0u,6u},
{49u,1u,4u,0u,7u},
{49u,1u,4u,0u,8u},
{49u,1u,4u,0u,9u},
{49u,1u,4u,0u,10u},
{49u,1u,4u,0u,11u},
{49u,1u,4u,0u,12u},
{49u,1u,4u,0u,13u},
{49u,1u,4u,0u,14u},
{49u,1u,4u,0u,15u},
{39u,1u,4u,0u,0u},
{40u,1u,4u,0u,0u},
{41u,1u,4u,0u,0u},
{42u,1u,4u,0u,0u},
{43u,1u,4u,0u,0u},
{44u,1u,4u,0u,0u},
{45u,1u,4u,0u,0u},
{46u,1u,4u,0u,0u},
{47u,1u,4u,0u,0u},
{16u,1u,4u,1u,0u},
{17u,1u,4u,1u,0u},
{18u,1u,4u,1u,0u},
{19u,1u,4u,1u,0u},
{20u,1u,4u,1u,0u},
{21u,1u,4u,1u,0u},
{22u,1u,4u,1u,0u},
{23u,1u,4u,1u,0u},
{24u,1u,4u,1u,0u},
{25u,1u,4u,1u,0u},
{26u,1u,4u,1u,0u},
{27u,1u,4u,1u,0u},
{28u,1u,4u,1u,0u},
{29u,1u,4u,1u,0u},
{30u,1u,4u,1u,0u},
{31u,1u,4u,1u,0u},
{32u,1u,4u,1u,0u},
{33u,1u,4u,1u,0u},
{48u,1u,4u,1u,0u},
{48u,1u,4u,1u,1u},
{48u,1u,4u,1u,2u},
{48u,1u,4u,1u,3u},
{48u,1u,4u,1u,4u},
{48u,1u,4u,1u,5u},
{48u,1u,4u,1u,6u},
{48u,1u,4u,1u,7u},
{34u,1u,4u,1u,0u},
{35u,1u,4u,1u,0u},
{36u,1u,4u,1u,0u},
{37u,1u,4u,1u,0u},
{38u,1u,4u,1u,0u},
{49u,1u,4u,1u,0u},
{49u,1u,4u,1u,1u},
{49u,1u,4u,1u,2u},
{49u,1u,4u,1u,3u},
{49u,1u,4u,1u,4u},
{49u,1u,4u,1u,5u},
{49u,1u,4u,1u,6u},
{49u,1u,4u,1u,7u},
{49u,1u,4u,1u,8u},
{49u,1u,4u,1u,9u},
{49u,1u,4u,1u,10u},
{49u,1u,4u,1u,11u},
{49u,1u,4u,1u,12u},
{49u,1u,4u,1u,13u},
{49u,1u,4u,1u,14u},
{49u,1u,4u,1u,15u},
{39u,1u,4u,1u,0u},
{40u,1u,4u,1u,0u},
{41u,1u,4u,1u,0u},
{42u,1u,4u,1u,0u},
{43u,1u,4u,1u,0u},
{44u,1u,4u,1u,0u},
{45u,1u,4u,1u,0u},
{46u,1u,4u,1u,0u},
{47u,1u,4u,1u,0u},
{16u,1u,4u,2u,0u},
{17u,1u,4u,2u,0u},
{18u,1u,4u,2u,0u},
{19u,1u,4u,2u,0u},
{20u,1u,4u,2u,0u},
{21u,1u,4u,2u,0u},
{22u,1u,4u,2u,0u},
{23u,1u,4u,2u,0u},
{24u,1u,4u,2u,0u},
{25u,1u,4u,2u,0u},
{26u,1u,4u,2u,0u},
{27u,1u,4u,2u,0u},
{28u,1u,4u,2u,0u},
{29u,1u,4u,2u,0u},
{30u,1u,4u,2u,0u},
{31u,1u,4u,2u,0u},
{32u,1u,4u,2u,0u},
{33u,1u,4u,2u,0u},
{48u,1u,4u,2u,0u},
{48u,1u,4u,2u,1u},
{48u,1u,4u,2u,2u},
{48u,1u,4u,2u,3u},
{48u,1u,4u,2u,4u},
{48u,1u,4u,2u,5u},
{48u,1u,4u,2u,6u},
{48u,1u,4u,2u,7u},
{34u,1u,4u,2u,0u},
{35u,1u,4u,2u,0u},
{36u,1u,4u,2u,0u},
{37u,1u,4u,2u,0u},
{38u,1u,4u,2u,0u},
{49u,1u,4u,2u,0u},
{49u,1u,4u,2u,1u},
{49u,1u,4u,2u,2u},
{49u,1u,4u,2u,3u},
{49u,1u,4u,2u,4u},
{49u,1u,4u,2u,5u},
{49u,1u,4u,2u,6u},
{49u,1u,4u,2u,7u},
{49u,1u,4u,2u,8u},
{49u,1u,4u,2u,9u},
{49u,1u,4u,2u,10u},
{49u,1u,4u,2u,11u},
{49u,1u,4u,2u,12u},
{49u,1u,4u,2u,13u},
{49u,1u,4u,2u,14u},
{49u,1u,4u,2u,15u},
{39u,1u,4u,2u,0u},
{40u,1u,4u,2u,0u},
{41u,1u,4u,2u,0u},
{42u,1u,4u,2u,0u},
{43u,1u,4u,2u,0u},
{44u,1u,4u,2u,0u},
{45u,1u,4u,2u,0u},
{46u,1u,4u,2u,0u},
{47u,1u,4u,2u,0u},
{16u,1u,4u,3u,0u},
{17u,1u,4u,3u,0u},
{18u,1u,4u,3u,0u},
{19u,1u,4u,3u,0u},
{20u,1u,4u,3u,0u},
{21u,1u,4u,3u,0u},
{22u,1u,4u,3u,0u},
{23u,1u,4u,3u,0u},
{24u,1u,4u,3u,0u},
{25u,1u,4u,3u,0u},
{26u,1u,4u,3u,0u},
{27u,1u,4u,3u,0u},
{28u,1u,4u,3u,0u},
{29u,1u,4u,3u,0u},
{30u,1u,4u,3u,0u},
{31u,1u,4u,3u,0u},
{32u,1u,4u,3u,0u},
{33u,1u,4u,3u,0u},
{48u,1u,4u,3u,0u},
{48u,1u,4u,3u,1u},
{48u,1u,4u,3u,2u},
{48u,1u,4u,3u,3u},
{48u,1u,4u,3u,4u},
{48u,1u,4u,3u,5u},
{48u,1u,4u,3u,6u},
{48u,1u,4u,3u,7u},
{34u,1u,4u,3u,0u},
{35u,1u,4u,3u,0u},
{36u,1u,4u,3u,0u},
{37u,1u,4u,3u,0u},
{38u,1u,4u,3u,0u},
{49u,1u,4u,3u,0u},
{49u,1u,4u,3u,1u},
{49u,1u,4u,3u,2u},
{49u,1u,4u,3u,3u},
{49u,1u,4u,3u,4u},
{49u,1u,4u,3u,5u},
{49u,1u,4u,3u,6u},
{49u,1u,4u,3u,7u},
{49u,1u,4u,3u,8u},
{49u,1u,4u,3u,9u},
{49u,1u,4u,3u,10u},
{49u,1u,4u,3u,11u},
{49u,1u,4u,3u,12u},
{49u,1u,4u,3u,13u},
{49u,1u,4u,3u,14u},
{49u,1u,4u,3u,15u},
{39u,1u,4u,3u,0u},
{40u,1u,4u,3u,0u},
{41u,1u,4u,3u,0u},
{42u,1u,4u,3u,0u},
{43u,1u,4u,3u,0u},
{44u,1u,4u,3u,0u},
{45u,1u,4u,3u,0u},
{46u,1u,4u,3u,0u},
{47u,1u,4u,3u,0u},
{5u,0u,0u,0u,0u},
{12u,2u,1u,0u,0u},
{13u,2u,1u,0u,0u},
{14u,2u,1u,0u,0u},
{15u,2u,1u,0u,0u},
{16u,2u,1u,0u,0u},
{17u,2u,1u,0u,0u},
{18u,2u,1u,0u,0u},
{19u,2u,1u,0u,0u},
{20u,2u,1u,0u,0u},
{21u,2u,1u,0u,0u},
{22u,2u,1u,0u,0u},
{23u,2u,1u,0u,0u},
{24u,2u,1u,0u,0u},
{25u,2u,1u,0u,0u},
{26u,2u,1u,0u,0u},
{27u,2u,1u,0u,0u},
{28u,2u,1u,0u,0u},
{29u,2u,1u,0u,0u},
{30u,2u,1u,0u,0u},
{31u,2u,1u,0u,0u},
{32u,2u,1u,0u,0u},
{33u,2u,1u,0u,0u},
{48u,2u,1u,0u,0u},
{48u,2u,1u,0u,1u},
{48u,2u,1u,0u,2u},
{48u,2u,1u,0u,3u},
{48u,2u,1u,0u,4u},
{48u,2u,1u,0u,5u},
{48u,2u,1u,0u,6u},
{48u,2u,1u,0u,7u},
{34u,2u,1u,0u,0u},
{35u,2u,1u,0u,0u},
{36u,2u,1u,0u,0u},
{37u,2u,1u,0u,0u},
{38u,2u,1u,0u,0u},
{49u,2u,1u,0u,0u},
{49u,2u,1u,0u,1u},
{49u,2u,1u,0u,2u},
{49u,2u,1u,0u,3u},
{49u,2u,1u,0u,4u},
{49u,2u,1u,0u,5u},
{49u,2u,1u,0u,6u},
{49u,2u,1u,0u,7u},
{49u,2u,1u,0u,8u},
{49u,2u,1u,0u,9u},
{49u,2u,1u,0u,10u},
{49u,2u,1u,0u,11u},
{49u,2u,1u,0u,12u},
{49u,2u,1u,0u,13u},
{49u,2u,1u,0u,14u},
{49u,2u,1u,0u,15u},
{39u,2u,1u,0u,0u},
{40u,2u,1u,0u,0u},
{41u,2u,1u,0u,0u},
{42u,2u,1u,0u,0u},
{43u,2u,1u,0u,0u},
{44u,2u,1u,0u,0u},
{45u,2u,1u,0u,0u},
{46u,2u,1u,0u,0u},
{47u,2u,1u,0u,0u},
{12u,2u,2u,0u,0u},
{13u,2u,2u,0u,0u},
{14u,2u,2u,0u,0u},
{15u,2u,2u,0u,0u},
{16u,2u,2u,0u,0u},
{17u,2u,2u,0u,0u},
{18u,2u,2u,0u,0u},
{19u,2u,2u,0u,0u},
{20u,2u,2u,0u,0u},
{21u,2u,2u,0u,0u},
{22u,2u,2u,0u,0u},
{23u,2u,2u,0u,0u},
{24u,2u,2u,0u,0u},
{25u,2u,2u,0u,0u},
{26u,2u,2u,0u,0u},
{27u,2u,2u,0u,0u},
{28u,2u,2u,0u,0u},
{29u,2u,2u,0u,0u},
{30u,2u,2u,0u,0u},
{31u,2u,2u,0u,0u},
{32u,2u,2u,0u,0u},
{33u,2u,2u,0u,0u},
{48u,2u,2u,0u,0u},
{48u,2u,2u,0u,1u},
{48u,2u,2u,0u,2u},
{48u,2u,2u,0u,3u},
{48u,2u,2u,0u,4u},
{48u,2u,2u,0u,5u},
{48u,2u,2u,0u,6u},
{48u,2u,2u,0u,7u},
{34u,2u,2u,0u,0u},
{35u,2u,2u,0u,0u},
{36u,2u,2u,0u,0u},
{37u,2u,2u,0u,0u},
{38u,2u,2u,0u,0u},
{49u,2u,2u,0u,0u},
{49u,2u,2u,0u,1u},
{49u,2u,2u,0u,2u},
{49u,2u,2u,0u,3u},
{49u,2u,2u,0u,4u},
{49u,2u,2u,0u,5u},
{49u,2u,2u,0u,6u},
{49u,2u,2u,0u,7u},
{49u,2u,2u,0u,8u},
{49u,2u,2u,0u,9u},
{49u,2u,2u,0u,10u},
{49u,2u,2u,0u,11u},
{49u,2u,2u,0u,12u},
{49u,2u,2u,0u,13u},
{49u,2u,2u,0u,14u},
{49u,2u,2u,0u,15u},
{39u,2u,2u,0u,0u},
{40u,2u,2u,0u,0u},
{41u,2u,2u,0u,0u},
{42u,2u,2u,0u,0u},
{43u,2u,2u,0u,0u},
{44u,2u,2u,0u,0u},
{45u,2u,2u,0u,0u},
{46u,2u,2u,0u,0u},
{47u,2u,2u,0u,0u},
{16u,2u,2u,1u,0u},
{17u,2u,2u,1u,0u},
{18u,2u,2u,1u,0u},
{19u,2u,2u,1u,0u},
{20u,2u,2u,1u,0u},
{21u,2u,2u,1u,0u},
{22u,2u,2u,1u,0u},
{23u,2u,2u,1u,0u},
{24u,2u,2u,1u,0u},
{25u,2u,2u,1u,0u},
{26u,2u,2u,1u,0u},
{27u,2u,2u,1u,0u},
{28u,2u,2u,1u,0u},
{29u,2u,2u,1u,0u},
{30u,2u,2u,1u,0u},
{31u,2u,2u,1u,0u},
{32u,2u,2u,1u,0u},
{33u,2u,2u,1u,0u},
{48u,2u,2u,1u,0u},
{48u,2u,2u,1u,1u},
{48u,2u,2u,1u,2u},
{48u,2u,2u,1u,3u},
{48u,2u,2u,1u,4u},
{48u,2u,2u,1u,5u},
{48u,2u,2u,1u,6u},
{48u,2u,2u,1u,7u},
{34u,2u,2u,1u,0u},
{35u,2u,2u,1u,0u},
{36u,2u,2u,1u,0u},
{37u,2u,2u,1u,0u},
{38u,2u,2u,1u,0u},
{49u,2u,2u,1u,0u},
{49u,2u,2u,1u,1u},
{49u,2u,2u,1u,2u},
{49u,2u,2u,1u,3u},
{49u,2u,2u,1u,4u},
{49u,2u,2u,1u,5u},
{49u,2u,2u,1u,6u},
{49u,2u,2u,1u,7u},
{49u,2u,2u,1u,8u},
{49u,2u,2u,1u,9u},
{49u,2u,2u,1u,10u},
{49u,2u,2u,1u,11u},
{49u,2u,2u,1u,12u},
{49u,2u,2u,1u,13u},
{49u,2u,2u,1u,14u},
{49u,2u,2u,1u,15u},
{39u,2u,2u,1u,0u},
{40u,2u,2u,1u,0u},
{41u,2u,2u,1u,0u},
{42u,2u,2u,1u,0u},
{43u,2u,2u,1u,0u},
{44u,2u,2u,1u,0u},
{45u,2u,2u,1u,0u},
{46u,2u,2u,1u,0u},
{47u,2u,2u,1u,0u},
{12u,2u,4u,0u,0u},
{13u,2u,4u,0u,0u},
{14u,2u,4u,0u,0u},
{15u,2u,4u,0u,0u},
{16u,2u,4u,0u,0u},
{17u,2u,4u,0u,0u},
{18u,2u,4u,0u,0u},
{19u,2u,4u,0u,0u},
{20u,2u,4u,0u,0u},
{21u,2u,4u,0u,0u},
{22u,2u,4u,0u,0u},
{23u,2u,4u,0u,0u},
{24u,2u,4u,0u,0u},
{25u,2u,4u,0u,0u},
{26u,2u,4u,0u,0u},
{27u,2u,4u,0u,0u},
{28u,2u,4u,0u,0u},
{29u,2u,4u,0u,0u},
{30u,2u,4u,0u,0u},
{31u,2u,4u,0u,0u},
{32u,2u,4u,0u,0u},
{33u,2u,4u,0u,0u},
{48u,2u,4u,0u,0u},
{48u,2u,4u,0u,1u},
{48u,2u,4u,0u,2u},
{48u,2u,4u,0u,3u},
{48u,2u,4u,0u,4u},
{48u,2u,4u,0u,5u},
{48u,2u,4u,0u,6u},
{48u,2u,4u,0u,7u},
{34u,2u,4u,0u,0u},
{35u,2u,4u,0u,0u},
{36u,2u,4u,0u,0u},
{37u,2u,4u,0u,0u},
{38u,2u,4u,0u,0u},
{49u,2u,4u,0u,0u},
{49u,2u,4u,0u,1u},
{49u,2u,4u,0u,2u},
{49u,2u,4u,0u,3u},
{49u,2u,4u,0u,4u},
{49u,2u,4u,0u,5u},
{49u,2u,4u,0u,6u},
{49u,2u,4u,0u,7u},
{49u,2u,4u,0u,8u},
{49u,2u,4u,0u,9u},
{49u,2u,4u,0u,10u},
{49u,2u,4u,0u,11u},
{49u,2u,4u,0u,12u},
{49u,2u,4u,0u,13u},
{49u,2u,4u,0u,14u},
{49u,2u,4u,0u,15u},
{39u,2u,4u,0u,0u},
{40u,2u,4u,0u,0u},
{41u,2u,4u,0u,0u},
{42u,2u,4u,0u,0u},
{43u,2u,4u,0u,0u},
{44u,2u,4u,0u,0u},
{45u,2u,4u,0u,0u},
{46u,2u,4u,0u,0u},
{47u,2u,4u,0u,0u},
{16u,2u,4u,1u,0u},
{17u,2u,4u,1u,0u},
{18u,2u,4u,1u,0u},
{19u,2u,4u,1u,0u},
{20u,2u,4u,1u,0u},
{21u,2u,4u,1u,0u},
{22u,2u,4u,1u,0u},
{23u,2u,4u,1u,0u},
{24u,2u,4u,1u,0u},
{25u,2u,4u,1u,0u},
{26u,2u,4u,1u,0u},
{27u,2u,4u,1u,0u},
{28u,2u,4u,1u,0u},
{29u,2u,4u,1u,0u},
{30u,2u,4u,1u,0u},
{31u,2u,4u,1u,0u},
{32u,2u,4u,1u,0u},
{33u,2u,4u,1u,0u},
{48u,2u,4u,1u,0u},
{48u,2u,4u,1u,1u},
{48u,2u,4u,1u,2u},
{48u,2u,4u,1u,3u},
{48u,2u,4u,1u,4u},
{48u,2u,4u,1u,5u},
{48u,2u,4u,1u,6u},
{48u,2u,4u,1u,7u},
{34u,2u,4u,1u,0u},
{35u,2u,4u,1u,0u},
{36u,2u,4u,1u,0u},
{37u,2u,4u,1u,0u},
{38u,2u,4u,1u,0u},
{49u,2u,4u,1u,0u},
{49u,2u,4u,1u,1u},
{49u,2u,4u,1u,2u},
{49u,2u,4u,1u,3u},
{49u,2u,4u,1u,4u},
{49u,2u,4u,1u,5u},
{49u,2u,4u,1u,6u},
{49u,2u,4u,1u,7u},
{49u,2u,4u,1u,8u},
{49u,2u,4u,1u,9u},
{49u,2u,4u,1u,10u},
{49u,2u,4u,1u,11u},
{49u,2u,4u,1u,12u},
{49u,2u,4u,1u,13u},
{49u,2u,4u,1u,14u},
{49u,2u,4u,1u,15u},
{39u,2u,4u,1u,0u},
{40u,2u,4u,1u,0u},
{41u,2u,4u,1u,0u},
{42u,2u,4u,1u,0u},
{43u,2u,4u,1u,0u},
{44u,2u,4u,1u,0u},
{45u,2u,4u,1u,0u},
{46u,2u,4u,1u,0u},
{47u,2u,4u,1u,0u},
{16u,2u,4u,2u,0u},
{17u,2u,4u,2u,0u},
{18u,2u,4u,2u,0u},
{19u,2u,4u,2u,0u},
{20u,2u,4u,2u,0u},
{21u,2u,4u,2u,0u},
{22u,2u,4u,2u,0u},
{23u,2u,4u,2u,0u},
{24u,2u,4u,2u,0u},
{25u,2u,4u,2u,0u},
{26u,2u,4u,2u,0u},
{27u,2u,4u,2u,0u},
{28u,2u,4u,2u,0u},
{29u,2u,4u,2u,0u},
{30u,2u,4u,2u,0u},
{31u,2u,4u,2u,0u},
{32u,2u,4u,2u,0u},
{33u,2u,4u,2u,0u},
{48u,2u,4u,2u,0u},
{48u,2u,4u,2u,1u},
{48u,2u,4u,2u,2u},
{48u,2u,4u,2u,3u},
{48u,2u,4u,2u,4u},
{48u,2u,4u,2u,5u},
{48u,2u,4u,2u,6u},
{48u,2u,4u,2u,7u},
{34u,2u,4u,2u,0u},
{35u,2u,4u,2u,0u},
{36u,2u,4u,2u,0u},
{37u,2u,4u,2u,0u},
{38u,2u,4u,2u,0u},
{49u,2u,4u,2u,0u},
{49u,2u,4u,2u,1u},
{49u,2u,4u,2u,2u},
{49u,2u,4u,2u,3u},
{49u,2u,4u,2u,4u},
{49u,2u,4u,2u,5u},
{49u,2u,4u,2u,6u},
{49u,2u,4u,2u,7u},
{49u,2u,4u,2u,8u},
{49u,2u,4u,2u,9u},
{49u,2u,4u,2u,10u},
{49u,2u,4u,2u,11u},
{49u,2u,4u,2u,12u},
{49u,2u,4u,2u,13u},
{49u,2u,4u,2u,14u},
{49u,2u,4u,2u,15u},
{39u,2u,4u,2u,0u},
{40u,2u,4u,2u,0u},
{41u,2u,4u,2u,0u},
{42u,2u,4u,2u,0u},
{43u,2u,4u,2u,0u},
{44u,2u,4u,2u,0u},
{45u,2u,4u,2u,0u},
{46u,2u,4u,2u,0u},
{47u,2u,4u,2u,0u},
{16u,2u,4u,3u,0u},
{17u,2u,4u,3u,0u},
{18u,2u,4u,3u,0u},
{19u,2u,4u,3u,0u},
{20u,2u,4u,3u,0u},
{21u,2u,4u,3u,0u},
{22u,2u,4u,3u,0u},
{23u,2u,4u,3u,0u},
{24u,2u,4u,3u,0u},
{25u,2u,4u,3u,0u},
{26u,2u,4u,3u,0u},
{27u,2u,4u,3u,0u},
{28u,2u,4u,3u,0u},
{29u,2u,4u,3u,0u},
{30u,2u,4u,3u,0u},
{31u,2u,4u,3u,0u},
{32u,2u,4u,3u,0u},
{33u,2u,4u,3u,0u},
{48u,2u,4u,3u,0u},
{48u,2u,4u,3u,1u},
{48u,2u,4u,3u,2u},
{48u,2u,4u,3u,3u},
{48u,2u,4u,3u,4u},
{48u,2u,4u,3u,5u},
{48u,2u,4u,3u,6u},
{48u,2u,4u,3u,7u},
{34u,2u,4u,3u,0u},
{35u,2u,4u,3u,0u},
{36u,2u,4u,3u,0u},
{37u,2u,4u,3u,0u},
{38u,2u,4u,3u,0u},
{49u,2u,4u,3u,0u},
{49u,2u,4u,3u,1u},
{49u,2u,4u,3u,2u},
{49u,2u,4u,3u,3u},
{49u,2u,4u,3u,4u},
{49u,2u,4u,3u,5u},
{49u,2u,4u,3u,6u},
{49u,2u,4u,3u,7u},
{49u,2u,4u,3u,8u},
{49u,2u,4u,3u,9u},
{49u,2u,4u,3u,10u},
{49u,2u,4u,3u,11u},
{49u,2u,4u,3u,12u},
{49u,2u,4u,3u,13u},
{49u,2u,4u,3u,14u},
{49u,2u,4u,3u,15u},
{39u,2u,4u,3u,0u},
{40u,2u,4u,3u,0u},
{41u,2u,4u,3u,0u},
{42u,2u,4u,3u,0u},
{43u,2u,4u,3u,0u},
{44u,2u,4u,3u,0u},
{45u,2u,4u,3u,0u},
{46u,2u,4u,3u,0u},
{47u,2u,4u,3u,0u},
{6u,0u,0u,0u,0u},
{7u,0u,0u,0u,0u},
{8u,0u,0u,0u,0u},
{9u,0u,0u,0u,0u},
{10u,0u,0u,0u,0u},
{11u,0u,0u,0u,0u},
};
inline constexpr Pass k_source[]={
{50u,0u,0u,0u,0u},
{51u,0u,0u,0u,0u},
{52u,0u,0u,0u,0u},
{53u,0u,0u,0u,0u},
{54u,0u,0u,0u,0u},
{55u,0u,0u,0u,0u},
{56u,0u,0u,0u,0u},
{57u,0u,0u,0u,0u},
{58u,0u,0u,0u,0u},
{59u,0u,0u,0u,0u},
{60u,0u,0u,0u,0u},
{61u,0u,0u,0u,0u},
{62u,0u,0u,0u,0u},
{9u,0u,0u,0u,0u},
{10u,0u,0u,0u,0u},
{63u,0u,0u,0u,0u},
};
struct Range {uint32_t offset,words;};
struct Profile {uint32_t nx,ny,nc,nf,sw,ww,pw,mgCount,groups; Range state[5],work[20]; uint32_t mg[6][3];};
inline constexpr Profile kProfiles[]={
{16u,16u,256u,544u,34080u,13392u,4160u,3u,9u,{{0u,256u},{256u,272u},{528u,272u},{800u,512u},{1312u,32768u}},{{0u,544u},{544u,544u},{1088u,544u},{1632u,544u},{2176u,544u},{2720u,544u},{3264u,544u},{3808u,544u},{4352u,544u},{4896u,544u},{5440u,672u},{6112u,672u},{6784u,672u},{7456u,672u},{8128u,256u},{8384u,256u},{8640u,256u},{8896u,256u},{9152u,144u},{9296u,4096u}},{{16u,16u,0u},{8u,8u,256u},{4u,4u,320u}}},
{32u,32u,1024u,2112u,37184u,40720u,16448u,4u,33u,{{0u,256u},{256u,1056u},{1312u,1056u},{2368u,2048u},{4416u,32768u}},{{0u,2112u},{2112u,2112u},{4224u,2112u},{6336u,2112u},{8448u,2112u},{10560u,2112u},{12672u,2112u},{14784u,2112u},{16896u,2112u},{19008u,2112u},{21120u,2720u},{23840u,2720u},{26560u,2720u},{29280u,2720u},{32000u,1024u},{33024u,1024u},{34048u,1024u},{35072u,1024u},{36096u,528u},{36624u,4096u}},{{32u,32u,0u},{16u,16u,1024u},{8u,8u,1280u},{4u,4u,1344u}}},
{64u,64u,4096u,8320u,49536u,149408u,65600u,5u,130u,{{0u,256u},{256u,4160u},{4416u,4160u},{8576u,8192u},{16768u,32768u}},{{0u,8320u},{8320u,8320u},{16640u,8320u},{24960u,8320u},{33280u,8320u},{41600u,8320u},{49920u,8320u},{58240u,8320u},{66560u,8320u},{74880u,8320u},{83200u,10912u},{94112u,10912u},{105024u,10912u},{115936u,10912u},{126848u,4096u},{130944u,4096u},{135040u,4096u},{139136u,4096u},{143232u,2080u},{145312u,4096u}},{{64u,64u,0u},{32u,32u,4096u},{16u,16u,5120u},{8u,8u,5376u},{4u,4u,5440u}}},
{64u,128u,8192u,16576u,65984u,294064u,131136u,5u,259u,{{0u,256u},{256u,8320u},{8576u,8256u},{16832u,16384u},{33216u,32768u}},{{0u,16576u},{16576u,16576u},{33152u,16576u},{49728u,16576u},{66304u,16576u},{82880u,16576u},{99456u,16576u},{116032u,16576u},{132608u,16576u},{149184u,16576u},{165760u,21824u},{187584u,21824u},{209408u,21824u},{231232u,21824u},{253056u,8192u},{261248u,8192u},{269440u,8192u},{277632u,8192u},{285824u,4144u},{289968u,4096u}},{{64u,128u,0u},{32u,64u,8192u},{16u,32u,10240u},{8u,16u,10752u},{4u,8u,10880u}}},
{128u,128u,16384u,33024u,98816u,582848u,262208u,6u,516u,{{0u,256u},{256u,16512u},{16768u,16512u},{33280u,32768u},{66048u,32768u}},{{0u,33024u},{33024u,33024u},{66048u,33024u},{99072u,33024u},{132096u,33024u},{165120u,33024u},{198144u,33024u},{231168u,33024u},{264192u,33024u},{297216u,33024u},{330240u,43680u},{373920u,43680u},{417600u,43680u},{461280u,43680u},{504960u,16384u},{521344u,16384u},{537728u,16384u},{554112u,16384u},{570496u,8256u},{578752u,4096u}},{{128u,128u,0u},{64u,64u,16384u},{32u,32u,20480u},{16u,16u,21504u},{8u,8u,21760u},{4u,4u,21824u}}},
};
inline constexpr char kHash_layoutSHA[]="b9390359ff66315b2d73506c4bf7001533f9f104b6780988ed3f0aa6b5e7cab9";
inline constexpr char kHash_gatesSHA[]="d6bbb2fe320786174cd3c81f84727f304b5fa73671f5fd83a90fbef9dff6dd48";
inline constexpr char kHash_mathSHA[]="1629bffe555eae801007af8ae7bc74e1a677b0b7a519853ff484a1f535d8a968";
inline constexpr char kHash_algorithmSHA[]="06b8881107a867dce309212532edadf823b679aafe0465ccf5933a2ab331395e";
const char* PipelineSource();
const char* PipelineSha();
bool PipelineTestHooks();
}
