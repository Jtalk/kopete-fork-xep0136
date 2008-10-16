#ifndef ALSA_IO
#define ALSA_IO

#include <alsa/asoundlib.h>

#include <QObject>
#include <QSocketNotifier>
#include <QFile>
#include <QTimer>

class Item
{
public:
	enum Type
	{
		Audio,
		Video
	};

	enum Direction
	{
		Input,
		Output
	};

	Type type;      // Audio or Video
	Direction dir;  // Input (mic) or Output (speaker)
	QString name;   // friendly name
	QString driver; // e.g. "oss", "alsa"
	QString id;     // e.g. "/dev/dsp", "hw:0,0"
};

QList<Item> getAlsaItems();

class AlsaItem
{
public:
	int card, dev;
	bool input;
	QString name;
};

class AlsaIO : public QObject
{
	Q_OBJECT
	Q_ENUMS(Format)
	Q_ENUMS(StreamType)
public:

	/** PCM sample format */
	enum Format {
		/** Unknown */
		Unknown = -1,
		/** Signed 8 bit */
		Signed8 = 0,
		/** Unsigned 8 bit */
		Unsigned8 = SND_PCM_FORMAT_U8,
		/** Signed 16 bit Little Endian */
		Signed16Le = SND_PCM_FORMAT_S16_LE,
		/** Signed 16 bit Big Endian */
		Signed16Be = SND_PCM_FORMAT_S16_BE,
		/** Unsigned 16 bit Little Endian */
		Unsigned16Le = SND_PCM_FORMAT_U16_LE,
		/** Unsigned 16 bit Big Endian */
		Unsigned16Be = SND_PCM_FORMAT_U16_BE,
		/** Mu-Law */
		MuLaw = SND_PCM_FORMAT_MU_LAW,
		/** A-Law */
		ALaw = SND_PCM_FORMAT_A_LAW
	};

	// Stream direction
	enum StreamType {
		Capture = 0,
		Playback
	};
	
	/*
	 * create and configure the alsa handle for the stream type t, the device
	 * dev and the sample format f
	 */
	AlsaIO(StreamType t, QString dev, Format f);
	~AlsaIO();

	/*
	 * returns the stream type currently used.
	 */
	StreamType type() const;

	/*
	 * start streaming, this must be called before starting playback too.
	 */
	bool start();
	
	/*
	 * writes raw audio data on the device.
	 * Actually, it fills a temporary buffer and write it on the device
	 * when the device is ready
	 */
	void write(const QByteArray& data);

	/*
	 * check if the device is ready. that means if it is correcly configured
	 * and ready to start streaming.
	 */
	bool isReady();
	
	/**
	 * @return period time in milisecond
	 */
	unsigned int periodTime() const;

	/**
	 * @return sampling rate
	 */
	unsigned int sRate() const;

	/**
	 * @return used format
	 */
	Format format() const {return m_format;}
	void setFormat(Format f);
	
	/*
	 * returns the data currently available.
	 * each time readyRead() is emitted, those data are
	 * dropped (if you have a reference on the data, it won't be dropped)
	 * and the next sample is available.
	 */
	QByteArray data();
	
	unsigned int timeStamp();
	void writeData();
	int frameSizeBytes();

public slots:
	void slotActivated(int socket);
	void checkAlsaPoll(int);

signals:
	void readyRead();
	void bytesWritten();

private:
	StreamType m_type;
	snd_pcm_t *handle;
	QSocketNotifier *notifier;
	bool ready;
	QByteArray buf;
	unsigned int pTime;
	snd_pcm_uframes_t pSize;
	unsigned int samplingRate;
	int fdCount;
	struct pollfd *ufds;
	unsigned int written;
	void stop();
	QFile *testFile;
	Format m_format;
	snd_pcm_hw_params_t *hwParams;
	int pSizeBytes;
	QByteArray tmpBuf;
	int times;
	
	bool prepare();
};

#endif //ALSA_IO
