#pragma once

#include "abstractRecognizer.h"
#include "KoskoNetwork/KoskoNetwork.h"

class koskoNetworkRecognizer : public AbstractRecognizer
{
public:
	koskoNetworkRecognizer(GesturesManager * recognizer,
						   const QMap<QString, PathVector> & objects);
	QString recognizeObject();
private:
	KoskoNetwork *mKosko;
	GesturesManager * mGesturesManager;
	QMap<QString, PathVector> mObjects;
	PathVector mGesture;
};
