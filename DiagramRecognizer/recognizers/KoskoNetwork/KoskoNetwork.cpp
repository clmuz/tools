#include "KoskoNetwork.h"
#include <fstream>
#include <cmath>

KoskoNetwork::KoskoNetwork(const QString &filename
						   , const int &width
						   , const int &xCentreMassPos
						   , const int &yCentreMassPos)
	: mWidth(width), mXCMPos(xCentreMassPos), mYCMPos(yCentreMassPos)
{
	QDomDocument kn;
	QFile knIn(filename);
	if (knIn.open(QIODevice::ReadOnly)) {
		if (kn.setContent(&knIn)) {
			QDomElement network = kn.documentElement();
			mCountOfANeurons = network.attribute("neuronsInALayer").toInt();
			mCountOfBNeurons = network.attribute("neuronsInBLayer").toInt();
			mCountOfEpochs = network.attribute("epochsNumber").toInt();
			mWeights.resize(mCountOfANeurons * mCountOfBNeurons * mCountOfEpochs);
			int e, i = 0;
			QDomElement name = network.firstChildElement("gestureName");
			mGesturesNames.clear();
			while (!name.isNull()) {
				mGesturesNames[name.attribute("number").toInt()] = name.attribute("name");
				name = name.nextSiblingElement("gestureName");
			}
			QDomElement epoch = network.firstChildElement("epoch");
			QDomElement line;
			for (e = 0; e < mCountOfEpochs; e++) {
				line = epoch.firstChildElement("line");
				for (i = 0; i < mCountOfBNeurons; i++) {
					loadLine(line, e, i);
					line = line.nextSiblingElement("line");
				}
				epoch = epoch.nextSiblingElement("epoch");
			}
		}
	}
	mHeight = mCountOfANeurons / mWidth;
	mANeuronLayer.fill(0, mCountOfANeurons);
	mBNeuronLayer.resize(mCountOfBNeurons);
	knIn.close();
}

int KoskoNetwork::getBNeuronNewValue(const int &BNeuronNum, const int &epoch) {
	int sum = 0;
	for (int i = 0; i < mCountOfANeurons; i++) {
		sum += mWeights[(epoch * mCountOfBNeurons + BNeuronNum) * mCountOfANeurons + i] * mANeuronLayer[i];
	}
	return sum;
}

int KoskoNetwork::getANeuronNewValue(const int &ANeuronNum, const int &epoch) {
	int sum = 0;
	for (int i = 0; i < mCountOfBNeurons; i++) {
		sum += mWeights[(epoch * mCountOfBNeurons + i) * mCountOfANeurons + ANeuronNum] * mBNeuronLayer[i];
	}
	return sum;
}

int KoskoNetwork::makeBipolar(const int &prevValue) {
	if (prevValue > 0) {
		return 1;
	}
	else {
		return -1;
	}
}

void KoskoNetwork::rememberALayer(QVector<int> &intVector) {
	for (int i = 0; i < mCountOfANeurons; i++) {
		intVector[i] = mANeuronLayer[i];
	}
}

void KoskoNetwork::rememberBLayer(QVector<int> &intVector) {
	for (int i = 0; i < mCountOfBNeurons; i++) {
		intVector[i] = mBNeuronLayer[i];
	}
}

void KoskoNetwork::fillALayer(const int &epoch) {
	for (int i = 0; i < mCountOfANeurons; i++) {
		mANeuronLayer[i] = makeBipolar(getANeuronNewValue(i, epoch));
	}
}

void KoskoNetwork::fillBLayer(const int &epoch) {
	for (int i = 0; i < mCountOfBNeurons; i++) {
		mBNeuronLayer[i] = makeBipolar(getBNeuronNewValue(i, epoch));
	}
}

QString KoskoNetwork::recognize(const PathVector &object)
{
	QVector<int> prevALayer(mCountOfANeurons, 0), prevBLayer(mCountOfBNeurons, 0);
	int countOfGestures = std::pow(2, mCountOfBNeurons);
	QVector<int> nowImg(mCountOfANeurons), resVector(countOfGestures, 0);
	makePath(object, nowImg);
	int i, e, result;
	for (i = 0; i < mCountOfANeurons; i++) {
		mANeuronLayer[i] = makeBipolar(nowImg[i]);
	}
	rememberALayer(prevALayer);
	for (e = 0; e < mCountOfEpochs; e++) {
		mANeuronLayer = prevALayer;
		fillBLayer(e);
		while (prevBLayer != mBNeuronLayer) {
			rememberBLayer(prevBLayer);
			fillALayer(e);
			fillBLayer(e);
		}
		result = 0;
		for (i = 0; i < mCountOfBNeurons; i++) {
			if (mBNeuronLayer[i] == 1) {
				result += std::pow(2, mCountOfBNeurons - i - 1);
			}
		}
		if (result >= e)
			resVector[result - e]++;
	}
	int max = 0;
	result = 0;
	for (i = 0; i < countOfGestures; i++) {
		if (resVector[i] > max) {
			max = resVector[i];
			result = i;
		}
	}
	if (result > mGesturesNames.size())
		return "";
	return mGesturesNames[result];
}

void KoskoNetwork::makePath(const PathVector &object, QVector<int> &nowImg)
{
	QVector<int> intPath;
	int leftBorder = INT_MAX;
	int upperBorder = INT_MIN;
	int rightBorder = INT_MIN;
	int bottomBorder = INT_MAX;
	nowImg.clear();
	int i = 0;
	foreach (PointVector const &stroke, object) {
		foreach (QPoint const &point, stroke) {
			leftBorder = std::min(leftBorder, point.x());
			upperBorder = std::min(upperBorder, point.y());
			rightBorder = std::max(rightBorder, point.x());
			bottomBorder = std::max(bottomBorder, point.y());
			intPath[i++] = point.x();
			intPath[i++] = point.y();
		}
	}
	for (i = 0; i < intPath.size(); i += 2) {
		intPath[i] -= leftBorder;
		intPath[i + 1] -= bottomBorder;
	}
	nowImg.fill(0);
	int width = rightBorder - leftBorder + 1;
	int height = upperBorder - bottomBorder + 1;
	QVector<int> img (width * height, 0);
	for (i = 0; i < intPath.size(); i += 2) {
		img[intPath[i + 1] * width + intPath[i]] = 1;
	}
	int leftCM, rightCM, upperCM, bottomCM;
	makeMassCentres(img, width, height, leftCM, rightCM, upperCM, bottomCM);
	int tmp;
	for (i = 0; i < intPath.size(); i += 2) {
		tmp = mXCMPos + (intPath[i] - leftCM) * ((double) (mWidth - 2 * mXCMPos)) / (rightCM - leftCM);
		intPath[i] = tmp;
		tmp = mYCMPos + (intPath[i + 1] - bottomCM) * ((double) (mHeight - 2 * mYCMPos)) / (upperCM - bottomCM);
		intPath[i + 1] = tmp;
	}
	nowImg.clear();
	nowImg.fill(0, mWidth * mHeight);
	for (i = 0; i < intPath.size(); i += 2) {
		nowImg[intPath[i + 1] * mWidth + intPath[i]] = 1;
	}
}

void KoskoNetwork::makeMassCentres(const QVector<int> &img
								 , const int &width
								 , const int &height
								 , int &leftCM
								 , int &rightCM
								 , int &upperCM
								 , int &bottomCM)
{
	int numOfDotsLeft = 0, numOfDotsRight = 0, numOfDotsUpper = 0, numOfDotsBottom = 0;
	int i, j;
	leftCM = 0;
	rightCM = 0;
	upperCM = 0;
	bottomCM = 0;
	for (i = 0; i <= width / 2; i++) {
		for (j = 0; j < height; j++) {
			leftCM += img[j * width + i] * i;
			numOfDotsLeft += img[j * width + i];
			rightCM += img[j * width + width - i - 1] * (width - i - 1);
			numOfDotsRight += img[j * width + width - i - 1];
		}
	}
	leftCM /= numOfDotsLeft;
	rightCM /= numOfDotsRight;
	for (j = 0; j <= height / 2; j++) {
		for (i = 0; i < width; i++) {
			bottomCM += img[j * width + i] * j;
			numOfDotsBottom += img[j * width + i];
			upperCM += img[(height - j - 1) * width + i] * (height - j - 1);
			numOfDotsUpper += img[(height - j - 1) * width + i];
		}
	}
	upperCM /= numOfDotsUpper;
	bottomCM /= numOfDotsBottom;
}

void KoskoNetwork::loadLine(const QDomElement &line, const int &epochNum, const int &lineNum)
{
	QString weightsStr = line.attribute("weights");
	int lenOfNum, nowWeight;
	for (int i = 0; i < mCountOfANeurons; i++) {
		lenOfNum = weightsStr.indexOf(",", 0);
		nowWeight = weightsStr.left(lenOfNum).toInt();
		mWeights[(epochNum * mCountOfBNeurons + lineNum) * mCountOfANeurons + i] = nowWeight;
		weightsStr = weightsStr.right(weightsStr.size() - lenOfNum - 2);
	}
}
