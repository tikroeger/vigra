#include "unittest.hxx"
#include <stdlib.h>
#include <algorithm>
#include <functional>
#include <vigra/stdimage.hxx>
#include <vigra/stdimagefunctions.hxx>
#include <vigra/functorexpression.hxx>
#include <vigra/fftw3.hxx>
#include <vigra/impex.hxx>
#include <vigra/inspectimage.hxx>
#include <vigra/gaborfilter.hxx>

using namespace std;
using namespace vigra;
using namespace vigra::functor;

struct Compare1
{
    double s;

    typedef vigra::FFTWComplexImage::value_type value_type;
    typedef vigra::FFTWComplexImage::value_type result_type;

    Compare1(double is)
    : s(is)
    {}

    result_type operator()(value_type v1, value_type v2) const
    { return v1 - v2/s; }
};

struct FFTWComplexTest
{
    FFTWComplex clx0, clx1, clx2, clx3;

    template <class VECTOR>
    void printVector(VECTOR const & v)
    {
        std::cerr << "(";
        for(int i=0; i<v.size(); ++i)
            std::cerr << v[i] << ", ";
        std::cerr << ")\n";
    }

    FFTWComplexTest()
    : clx0(), clx1(2.0), clx2(0.0, -2.0), clx3(2.0, -2.0)
    {}

    void testConstruction()
    {
        shouldEqual(clx0.re() , 0.0);
        shouldEqual(clx0.im() , 0.0);
        shouldEqual(clx1.re() , 2.0);
        shouldEqual(clx1.im() , 0.0);
        shouldEqual(clx2.re() , 0.0);
        shouldEqual(clx2.im() , -2.0);
        shouldEqual(clx3.re() , 2.0);
        shouldEqual(clx3.im() , -2.0);
        shouldEqual(clx0[0] , 0.0);
        shouldEqual(clx0[1] , 0.0);
        shouldEqual(clx1[0] , 2.0);
        shouldEqual(clx1[1] , 0.0);
        shouldEqual(clx2[0] , 0.0);
        shouldEqual(clx2[1] , -2.0);
        shouldEqual(clx3[0] , 2.0);
        shouldEqual(clx3[1] , -2.0);
    }

    void testComparison()
    {
        should(clx0 == FFTWComplex(0.0, 0.0));
        should(clx1 == FFTWComplex(2.0, 0.0));
        should(clx2 == FFTWComplex(0.0, -2.0));
        should(clx3 == FFTWComplex(2.0, -2.0));
        should(clx0 != clx1);
        should(clx0 != clx2);
        should(clx1 != clx2);
        should(clx1 != clx3);
    }

    void testArithmetic()
    {
        shouldEqual(abs(clx0) , 0.0);
        shouldEqual(abs(clx1) , 2.0);
        shouldEqual(abs(clx2) , 2.0);
        shouldEqual(abs(clx3) , sqrt(8.0));
        should(conj(clx3) == FFTWComplex(2.0, 2.0));

        shouldEqual(clx1.phase() , 0.0);
        shouldEqual(sin(clx2.phase()) , -1.0);
        shouldEqualTolerance(sin(clx3.phase()), -sqrt(2.0)/2.0, 1.0e-7);

        should(FFTWComplex(2.0, -2.0) == clx1 + clx2);

        should(clx0 == clx1 - clx1);
        should(clx0 == clx2 - clx2);
        should(FFTWComplex(2.0, 2.0) == clx1 - clx2);

        should(2.0*clx1 == FFTWComplex(4.0, 0.0));
        should(2.0*clx2 == FFTWComplex(0.0, -4.0));
        should(clx1*clx2 == FFTWComplex(0.0, -4.0));
        should(clx3*conj(clx3) == clx3.squaredMagnitude());

        should(clx1/2.0 == FFTWComplex(1.0, 0.0));
        should(clx2/2.0 == FFTWComplex(0.0, -1.0));
        should(clx2/clx1 == FFTWComplex(0.0, -1.0));
        should(clx3*conj(clx3) == clx3.squaredMagnitude());

    }

    void testAccessor()
    {
        FFTWComplexImage img(2,2);
        img = clx2;
        img(0,0) = clx3;
        FFTWComplexImage::Iterator i = img.upperLeft();

        FFTWComplexImage::Accessor get = img.accessor();
        FFTWRealAccessor real;
        FFTWImaginaryAccessor imag;
        FFTWMagnitudeAccessor mag;
        FFTWPhaseAccessor phase;
        FFTWWriteRealAccessor writeReal;

        should(get(i) == clx3);
        should(get(i, Diff2D(1,1)) == clx2);
        should(real(i) == 2.0);
        should(real(i, Diff2D(1,1)) == 0.0);
        should(imag(i) == -2.0);
        should(imag(i, Diff2D(1,1)) == -2.0);
        shouldEqual(mag(i) , sqrt(8.0));
        shouldEqual(mag(i, Diff2D(1,1)) , 2.0);
        shouldEqualTolerance(sin(phase(i)), -sqrt(2.0)/2.0, 1.0e-7);
        shouldEqual(sin(phase(i, Diff2D(1,1))), -1.0);

        writeReal.set(2.0, i);
        writeReal.set(2.0, i, Diff2D(1,1));
        should(get(i) == clx1);
        should(get(i, Diff2D(1,1)) == clx1);
    }

    void testForewardBackwardTrans()
    {
        const int w=256, h=256;

        vigra::FFTWComplexImage in(w, h);
        for (int y=0; y<in.height(); y++)
            for (int x=0; x<in.width(); x++)
            {
                in(x,y)= rand()/(double)RAND_MAX;
            }

        vigra::FFTWComplexImage out(w, h);

        fftw_plan  forwardPlan=
            fftw_plan_dft_2d(w, h, (fftw_complex *)in.begin(), (fftw_complex *)out.begin(), 
                             FFTW_FORWARD, FFTW_ESTIMATE );
        fftw_plan  backwardPlan=
            fftw_plan_dft_2d(w, h, (fftw_complex *)out.begin(), (fftw_complex *)out.begin(), 
                             FFTW_BACKWARD, FFTW_ESTIMATE);

        fftw_execute(forwardPlan);
        fftw_execute(backwardPlan);

        vigra::FindAverage<FFTWComplex::value_type> average;
        inspectImage(srcImageRange(out, vigra::FFTWImaginaryAccessor()), average);

        shouldEqualTolerance(average(), 0.0, 1e-14);

        combineTwoImages(srcImageRange(in), srcImage(out), destImage(out),
                         Compare1((double)w*h));

        average = vigra::FindAverage<vigra::FFTWMagnitudeAccessor::value_type>();
        inspectImage(srcImageRange(out, vigra::FFTWMagnitudeAccessor()), average);

        shouldEqualTolerance(average(), 0.0, 1e-14);

        for (int y=0; y<in.height(); y++)
            for (int x=0; x<in.width(); x++)
            {
                in(x,y)[1]= rand()/(double)RAND_MAX;
            }

        fftw_execute(forwardPlan);
        fftw_execute(backwardPlan);

        combineTwoImages(srcImageRange(in), srcImage(out), destImage(out),
                         Compare1((double)w*h));

        vigra::FindAverage<FFTWComplex> caverage;
        inspectImage(srcImageRange(out), caverage);

        shouldEqualTolerance(caverage().magnitude(), 0.0, 1e-14);

        fftw_destroy_plan(forwardPlan);
        fftw_destroy_plan(backwardPlan);
    }

    void testRearrangeQuadrants()
    {
        double t4[] = { 0, 1, 2, 2,
                        1, 1, 2, 2,
                        3, 3, 4, 4,
                        3, 3, 4, 4};
        double t4res[] = { 4, 4, 3, 3,
                           4, 4, 3, 3,
                           2, 2, 0, 1,
                           2, 2, 1, 1};

        FFTWComplexImage in4(4,4), out4(4,4);
        copy(t4, t4+16, in4.begin());
        moveDCToCenter(srcImageRange(in4), destImage(out4));
        moveDCToUpperLeft(srcImageRange(out4), destImage(in4));
        for(int i=0; i<16; ++i)
        {
            should(out4.begin()[i] == t4res[i]);
            should(in4.begin()[i] == t4[i]);
        }

        double t5[] = { 0, 1, 1, 2, 2,
                        1, 1, 1, 2, 2,
                        1, 1, 1, 2, 2,
                        3, 3, 3, 4, 4,
                        3, 3, 3, 4, 4};
        double t5res[] = { 4, 4, 3, 3, 3,
                           4, 4, 3, 3, 3,
                           2, 2, 0, 1, 1,
                           2, 2, 1, 1, 1,
                           2, 2, 1, 1, 1};

        FFTWComplexImage in5(5,5), out5(5,5);
        copy(t5, t5+25, in5.begin());
        moveDCToCenter(srcImageRange(in5), destImage(out5));
        moveDCToUpperLeft(srcImageRange(out5), destImage(in5));
        for(int i=0; i<25; ++i)
        {
            should(out5.begin()[i] == t5res[i]);
            should(in5.begin()[i] == t5[i]);
        }
    }
};

struct FFTWTestSuite
: public vigra::test_suite
{

    FFTWTestSuite()
    : vigra::test_suite("FFTWTest")
    {

        add( testCase(&FFTWComplexTest::testConstruction));
        add( testCase(&FFTWComplexTest::testComparison));
        add( testCase(&FFTWComplexTest::testArithmetic));
        add( testCase(&FFTWComplexTest::testAccessor));
        add( testCase(&FFTWComplexTest::testForewardBackwardTrans));
        add( testCase(&FFTWComplexTest::testRearrangeQuadrants));

    }
};

int initializing;

struct CompareFunctor
{
	double sumDifference_;

	CompareFunctor(): sumDifference_(0) {}

	void operator()(const float &a, const float &b)
		{ sumDifference_+= abs(a-b); }

    double operator()()
		{ return sumDifference_; }
};

struct GaborTests
{
	ImageImportInfo info;
	int w, h;
	FImage image;

	GaborTests()
		: info("../../images/ghouse.gif"),
		  w(info.width()), h(info.height()),
		  image(w, h)
	{
		importImage(info, destImage(image));
	}

	template<class Iterator, class Accessor>
	void checkImage(triple<Iterator, Iterator, Accessor> src, char *filename)
	{
		if (initializing)
		{
			exportImage(src, ImageExportInfo(filename));
			cout << "wrote " << filename << endl;
		}
		else
		{
			ImageImportInfo info(filename);
			FImage image(info.width(), info.height());
			importImage(info, destImage(image));

			CompareFunctor cmp;

			inspectTwoImages(src, srcImage(image), cmp);
			cout << "difference to " << filename << ": " << cmp() << endl;
			shouldEqualTolerance(cmp(), 0.0, 1e-4);
		}
	}

	void testImages()
	{
		FImage filter(w, h);
		int directionCount= 8;
		int dir= 1, scale= 1;
		double angle = dir * M_PI / directionCount;
		double centerFrequency = 3.0/8.0 / VIGRA_CSTD::pow(2.0,scale);
		createGaborFilter(destImageRange(filter),
						  angle, centerFrequency,
						  angularGaborSigma(directionCount, centerFrequency),
						  radialGaborSigma(centerFrequency));

		checkImage(srcImageRange(filter), "filter.tif");

		cout << "Applying filter...\n";
		FFTWComplexImage result(w, h);
		applyFourierFilter(srcImageRange(image), srcImage(filter), destImage(result));
		checkImage(srcImageRange(result, FFTWMagnitudeAccessor()), "gaborresult.tif");

		FImage realPart(w, h);
		applyFourierFilter(srcImageRange(image), srcImage(filter), destImage(realPart));

		CompareFunctor cmp;
		inspectTwoImages(srcImageRange(result, FFTWRealAccessor()),
						 srcImage(realPart), cmp);
		cout << "difference between real parts: " << cmp() << endl;
		shouldEqualTolerance(cmp(), 0.0, 1e-4);

		cout << "testing vector results..\n";
		FVector2Image vectorResult(w,h);
		applyFourierFilter(srcImageRange(image), srcImage(filter),
						   destImage(vectorResult));
		CompareFunctor iCmp;
		inspectTwoImages(srcImageRange(result, FFTWImaginaryAccessor()),
						 srcImage(vectorResult,
								  VectorComponentAccessor<FVector2Image::PixelType>(1)),
						 iCmp);
		cout << "difference between imaginary parts: " << iCmp() << endl;
		shouldEqualTolerance(cmp(), 0.0, 1e-4);

		cout << "applying on ROI...\n";
		FImage bigImage(w+20, h+20);
		FImage::Iterator bigUL= bigImage.upperLeft() + Diff2D(5, 5);
		copyImage(srcImageRange(image), destIter(bigUL));
		applyFourierFilter(srcIterRange(bigUL, bigUL + Diff2D(w, h)),
						   srcImage(filter), destImage(result));
		checkImage(srcImageRange(result, FFTWMagnitudeAccessor()), "gaborresult.tif");
#if 0
		cout << "Creating plans with measurement...\n";
		fftwnd_plan forwardPlan=
			fftw2d_create_plan(h, w, FFTW_FORWARD, FFTW_MEASURE );
		fftwnd_plan backwardPlan=
			fftw2d_create_plan(h, w, FFTW_BACKWARD, FFTW_MEASURE | FFTW_IN_PLACE);

		cout << "Applying again...\n";
		applyFourierFilter(srcImageRange(image), srcImage(filter), destImage(result),
						   forwardPlan, backwardPlan);

		checkImage(srcImageRange(result, FFTWMagnitudeAccessor()), "gaborresult.tif");
#endif
	}

	void testFamily()
	{
		cout << "testing 8x2 GaborFilterFamily...\n";
		GaborFilterFamily<FImage> filters(w, h, 8, 2);
		ImageArray<FFTWComplexImage> results((unsigned int)filters.size(), filters.imageSize());
		applyFourierFilterFamily(srcImageRange(image), filters, results);
		checkImage(srcImageRange(results[filters.filterIndex(1,1)], FFTWMagnitudeAccessor()),
				   "gaborresult.tif");

		ImageArray<FImage> realParts((unsigned int)filters.size(), filters.imageSize());
		applyFourierFilterFamily(srcImageRange(image), filters, realParts);

		CompareFunctor cmp;
		inspectTwoImages(srcImageRange(results[3], FFTWRealAccessor()),
						 srcImage(realParts[3]), cmp);
		cout << "difference between real parts: " << cmp() << endl;
		shouldEqualTolerance(cmp(), 0.0, 1e-4);
	}
};

struct GaborTestSuite
: public vigra::test_suite
{
    GaborTestSuite()
    : vigra::test_suite("FFTWWithGaborTest")
    {
        add(testCase(&GaborTests::testImages));
        add(testCase(&GaborTests::testFamily));
    }
};

int main(int argc, char **argv)
{
    initializing= argc>1; // global variable,
    // initializing means: don't compare with image files but create them

    FFTWTestSuite fftwTest;

    int failed = fftwTest.run();

    std::cout << fftwTest.report() << std::endl;

    GaborTestSuite gaborTest;

    failed = failed || gaborTest.run();

    std::cout << gaborTest.report() << std::endl;

    return (failed != 0);
}