#include "RenderUtil.h"
#include "DxLib.h"
#include <algorithm>

namespace RenderUtil
{
	void DrawGlowBox(int x1, int y1, int x2, int y2, int color, QualityLevel q)
	{
		int layers = 4;
		int maxExpand = 18;
		int maxAlpha = 45;

		if (q == QualityLevel::Low)
		{
			layers = 2;
			maxExpand = 8;
			maxAlpha = 45;
		}

		SetDrawBlendMode(DX_BLENDMODE_ADD, 0);

		for (int i = 0; i < layers; ++i)
		{
			const float t = (layers <= 1) ? 1.0f : (float)i / (layers - 1);
			const int expand = (int)(t * maxExpand);

			int alpha = (int)(maxAlpha * (1.0f - t));
			alpha = std::clamp(alpha, 10, maxAlpha);

			SetDrawBlendMode(DX_BLENDMODE_ADD, alpha);
			DrawBox(x1 - expand, y1 - expand, x2 + expand, y2 + expand, color, TRUE);
		}

		SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	}
}