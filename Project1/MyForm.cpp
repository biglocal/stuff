#include "MyForm.h"
// Standard Library
#include <array>
#include <iostream>

// OpenCV Header
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

// Kinect for Windows SDK Header
#include <Kinect.h>
using namespace System;
using namespace System::Windows::Forms;

[STAThread]//leave this as is
void main(array<String^>^ args) {
	Application::EnableVisualStyles();
	Application::SetCompatibleTextRenderingDefault(false);
	Project1::MyForm form;
	Application::Run(%form);
}

void Project1::MyForm::btn_start_Click(System::Object^  sender, System::EventArgs^  e) {
	lb_status->Text = L"Hello";
	// 1. Sensor related code
	// 1a. Get default Sensor
	if (btn_start->Text == "Start")
	{
		btn_start->Text = "Stop";
	}
	else
		btn_start->Text = "Start";
	
	lb_log->Items->Add("open sensor");
	IKinectSensor* pSensor = nullptr;
	{
		if (GetDefaultKinectSensor(&pSensor) != S_OK)
		{
			lb_log->Items->Add("Get Sensor failed");
			return;
		}

		lb_log->Items->Add("Try to open sensor");
		if (pSensor->Open() != S_OK)
		{
			lb_log->Items->Add("Can't open sensor");
			return;
		}
	}

	// 3. Depth related code
	IDepthFrameReader* pDepthFrameReader = nullptr;
	UINT uDepthPointNum = 0;
	int iDepthWidth = 0, iDepthHeight = 0;
	UINT16 uDepthMin = 0, uDepthMax = 0;
	lb_log->Items->Add("Try to get depth source");
	{
		// Get frame source
		IDepthFrameSource* pFrameSource = nullptr;
		if (pSensor->get_DepthFrameSource(&pFrameSource) != S_OK)
		{
			lb_log->Items->Add("Can't get depth frame source");
			return;
		}

		// Get frame description
		lb_log->Items->Add("get depth frame description");
		IFrameDescription* pFrameDescription = nullptr;
		if (pFrameSource->get_FrameDescription(&pFrameDescription) == S_OK)
		{
			pFrameDescription->get_Width(&iDepthWidth);
			pFrameDescription->get_Height(&iDepthHeight);
			uDepthPointNum = iDepthWidth * iDepthHeight;
		}
		pFrameSource->get_DepthMinReliableDistance(&uDepthMin);
		pFrameSource->get_DepthMaxReliableDistance(&uDepthMax);
		pFrameDescription->Release();
		pFrameDescription = nullptr;

		// get frame reader
		lb_log->Items->Add("Try to get depth frame reader");
		if (pFrameSource->OpenReader(&pDepthFrameReader) != S_OK)
		{
			lb_log->Items->Add("Can't get depth frame reader");
			return;
		}

		// release Frame source
		lb_log->Items->Add("Release frame source");
		pFrameSource->Release();
		pFrameSource = nullptr;
	}

	IBodyFrameReader* pBDFrameReader = nullptr;
	INT32 iBodyCount = 0;
	IBody** aBodyData = nullptr;
	lb_log->Items->Add("Try to get body joint source");
	{
		IBodyFrameSource* pFrameSource = nullptr;
		if (pSensor->get_BodyFrameSource(&pFrameSource) != S_OK)
		{
			lb_log->Items->Add("Can't get body frame source");
			return;
		}

		if (pFrameSource->get_BodyCount(&iBodyCount) != S_OK)
		{
			lb_log->Items->Add("Can't get body count");
			return;
		}

		lb_log->Items->Add("Can trace 6 bodies");
		aBodyData = new IBody*[iBodyCount];
		for (int i = 0; i < iBodyCount; ++i)
			aBodyData[i] = nullptr;

		// 3a. get frame reader
		lb_log->Items->Add("Try to get body frame reader");
		if (pFrameSource->OpenReader(&pBDFrameReader) != S_OK)
		{
			lb_log->Items->Add("Can't get body frame reader");
			return;
		}
		lb_log->Items->Add("Release frame source");
		pFrameSource->Release();
		pFrameSource = nullptr;
	}

	IBodyIndexFrameReader* pBIFrameReader = nullptr;
	lb_log->Items->Add("Try to get body index source");
	{
		// Get frame source
		IBodyIndexFrameSource* pFrameSource = nullptr;
		if (pSensor->get_BodyIndexFrameSource(&pFrameSource) != S_OK)
		{
			lb_log->Items->Add("Can't get body index frame source");
			return;
		}

		// get frame reader
		lb_log->Items->Add("Try to get body index frame reader");
		if (pFrameSource->OpenReader(&pBIFrameReader) != S_OK)
		{
			lb_log->Items->Add("Can't get depth frame reader");
			return;
		}

		// release Frame source
		lb_log->Items->Add("Release frame source");
		pFrameSource->Release();
		pFrameSource = nullptr;
	}

	// 5. Coordinate Mapper
	ICoordinateMapper* pCoordinateMapper = nullptr;
	if (pSensor->get_CoordinateMapper(&pCoordinateMapper) != S_OK)
	{
		lb_log->Items->Add("get_CoordinateMapper failed");
		return;
	}

	// perpare OpenCV
	cv::Mat	mRightHandImg;
	cv::Mat mDepthImg(iDepthHeight, iDepthWidth, CV_16UC1);
	cv::Mat mImg8bit(iDepthHeight, iDepthWidth, CV_8UC1);
	cv::namedWindow("Depth Map");

	// 3a. get frame reader
	lb_log->Items->Add("Enter main loop");
	while (true)
	{
		// 4a. Get last frame
		IDepthFrame* pFrame = nullptr;
		UINT16*		pDepthPoints = new UINT16[uDepthPointNum];
		bool track_r = FALSE, track_l = FALSE;
		int distance_r, distance_l;
		Joint aJoints[JointType::JointType_Count];
		BYTE*	pBodyIndex = new BYTE[uDepthPointNum];
		if (pDepthFrameReader->AcquireLatestFrame(&pFrame) == S_OK)
		{
			// 4c. copy the depth map to image
			if (pFrame->CopyFrameDataToArray(iDepthWidth * iDepthHeight, pDepthPoints) != S_OK)
			{
				lb_log->Items->Add("Data copy error");
				//mDepthImg = cv::Mat(iDepthHeight, iDepthWidth, CV_16U, pDepthPoints);
				//mDepthImg.convertTo(mImg8bit, CV_8U, 255.0f / uDepthMax);

			}
			// 4e. release frame
			pFrame->Release();
		}

		IBodyIndexFrame* pBIFrame = nullptr;
		if (pBIFrameReader->AcquireLatestFrame(&pBIFrame) == S_OK)
		{
			pBIFrame->CopyFrameDataToArray(uDepthPointNum, pBodyIndex);
			pBIFrame->Release();
			pBIFrame = nullptr;
		}

		IBodyFrame* pBDFrame = nullptr;
		if (pBDFrameReader->AcquireLatestFrame(&pBDFrame) == S_OK)
		{
			if (pBDFrame->GetAndRefreshBodyData(iBodyCount, aBodyData) == S_OK)
			{
				int iTrackedBodyCount = 0;
				DepthSpacePoint ptJ1, ptJ2;
				for (int i = 0; i < iBodyCount; ++i)
				{
					IBody* pBody = aBodyData[i];
					// check if is tracked
					BOOLEAN bTracked = false;
					if ((pBody->get_IsTracked(&bTracked) == S_OK) && bTracked)
					{
						++iTrackedBodyCount;
						int x, y, tipx, tipy;

						//4: handtipRight, HandTipLeft, HandRight, HandLeft
						DepthSpacePoint *depthSpacePosition = new DepthSpacePoint[JointType::JointType_Count];

						if (pBody->GetJoints(JointType::JointType_Count, aJoints) == S_OK)
						{

							pCoordinateMapper->MapCameraPointToDepthSpace(aJoints[JointType_HandRight].Position, &depthSpacePosition[JointType_HandRight]);
							pCoordinateMapper->MapCameraPointToDepthSpace(aJoints[JointType_HandTipRight].Position, &depthSpacePosition[JointType_HandTipRight]);
							pCoordinateMapper->MapCameraPointToDepthSpace(aJoints[JointType_HandLeft].Position, &depthSpacePosition[JointType_HandLeft]);
							pCoordinateMapper->MapCameraPointToDepthSpace(aJoints[JointType_HandTipLeft].Position, &depthSpacePosition[JointType_HandTipLeft]);
							//DrawLine(mImg8bit, aJoints[JointType_HandRight], aJoints[JointType_HandTipRight], pCoordinateMapper, &ptJ1, &ptJ2);
							//Drawrectangle_depth(mImg8bit, &depthSpacePosition[JointType_HandRight], &depthSpacePosition[JointType_HandTipRight]);

							int hand_r_x = depthSpacePosition[JointType_HandRight].X;
							int hand_r_y = depthSpacePosition[JointType_HandRight].Y;
							int hand_l_x = depthSpacePosition[JointType_HandLeft].X;
							int hand_l_y = depthSpacePosition[JointType_HandLeft].Y;
							int depth_value_r;
							int depth_value_l;


							if (aJoints[JointType_HandRight].TrackingState == TrackingState_Tracked)
							{
								depth_value_r = hand_r_x + hand_r_y * iDepthWidth;
								if (depth_value_r >= 0 && depth_value_r < iDepthHeight * iDepthWidth)
								{
									distance_r = pDepthPoints[depth_value_r] >> 3;
									track_r = TRUE;
								}
							}
							if (aJoints[JointType_HandLeft].TrackingState == TrackingState_Tracked)
							{
								depth_value_l = hand_l_x + hand_l_y * iDepthWidth;
								if (depth_value_l >= 0 && depth_value_l < iDepthHeight * iDepthWidth)
								{
									distance_l = pDepthPoints[depth_value_l] >> 3;
									track_l = TRUE;
								}
							}
						}
					}
				}
			}
			pBDFrame->Release();
		}
		int range = 7;
		for (int idx = 0; idx < iDepthHeight * iDepthWidth; idx++)
		{
			/*0xfff8 for white, 0x08 for black*/
			if (track_r == TRUE && pDepthPoints[idx] >> 3 <= (distance_r + range) && pDepthPoints[idx] >> 3 >= (distance_r - range) && pBodyIndex[idx] < 6)
			{
				pDepthPoints[idx] &= 0x08;
			}
			/*
			else if (track_l == TRUE && pDepthPoints[idx] >> 3 <= (distance_l + range) && pDepthPoints[idx] >> 3 >= (distance_l - range)&& pBodyIndex[idx] < 6)
			{
			pDepthPoints[idx] &= 0x08;
			}
			*/
			else
				pDepthPoints[idx] |= 0xfff8;

		}
		mDepthImg = cv::Mat(iDepthHeight, iDepthWidth, CV_16U, pDepthPoints);
		mDepthImg.convertTo(mImg8bit, CV_8U, 255.0f / uDepthMax);
		cv::imshow("Depth Map", mImg8bit);

		// 4f. check keyboard input
		if (cv::waitKey(60) == VK_ESCAPE || btn_start->Text=="Start") {
			break;
		}
	}

	// 3b. release frame reader
	lb_log->Items->Add("Release frame reader");
	pDepthFrameReader->Release();
	pDepthFrameReader = nullptr;

	// 1c. Close Sensor
	lb_log->Items->Add("close sensor");
	pSensor->Close();


	// 1d. Release Sensor
	lb_log->Items->Add("Release sensor");
	pSensor->Release();
	pSensor = nullptr;
	return;
}