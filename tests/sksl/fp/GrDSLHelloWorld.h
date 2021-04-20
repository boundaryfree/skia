/* HELLO WORLD */

/**************************************************************************************************
 *** This file was autogenerated from GrDSLHelloWorld.fp; do not modify.
 **************************************************************************************************/
#ifndef GrDSLHelloWorld_DEFINED
#define GrDSLHelloWorld_DEFINED

#include "include/core/SkM44.h"
#include "include/core/SkTypes.h"


#include "src/gpu/GrFragmentProcessor.h"

class GrDSLHelloWorld : public GrFragmentProcessor {
public:
    static std::unique_ptr<GrFragmentProcessor> Make() {
        return std::unique_ptr<GrFragmentProcessor>(new GrDSLHelloWorld());
    }
    GrDSLHelloWorld(const GrDSLHelloWorld& src);
    std::unique_ptr<GrFragmentProcessor> clone() const override;
    const char* name() const override { return "DSLHelloWorld"; }
private:
    GrDSLHelloWorld()
    : INHERITED(kGrDSLHelloWorld_ClassID, kNone_OptimizationFlags) {
    }
    std::unique_ptr<GrGLSLFragmentProcessor> onMakeProgramImpl() const override;
    void onGetGLSLProcessorKey(const GrShaderCaps&, GrProcessorKeyBuilder*) const override;
    bool onIsEqual(const GrFragmentProcessor&) const override;
#if GR_TEST_UTILS
    SkString onDumpInfo() const override;
#endif
    GR_DECLARE_FRAGMENT_PROCESSOR_TEST
    using INHERITED = GrFragmentProcessor;
};
#endif