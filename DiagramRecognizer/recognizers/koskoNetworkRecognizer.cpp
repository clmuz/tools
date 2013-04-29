#include "koskoNetworkRecognizer.h"

koskoNetworkRecognizer::koskoNetworkRecognizer(GesturesManager * recognizer,
											   const QMap<QString, PathVector> & objects) :
	AbstractRecognizer(recognizer, objects)
{
	mKosko = new KoskoNetwork("KoskoNetworkWeights.xml", 80, 30, 30);
}

QString koskoNetworkRecognizer::recognizeObject()
{
	return mKosko->recognize(mGesture);
}
