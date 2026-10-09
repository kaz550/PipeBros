#pragma once

class Input;

class IScene
{
public:
	virtual ~IScene() = default;

	virtual void Enter() {}
	virtual void Exit() {}

	virtual void Update(float dt, const Input& input) = 0;
	virtual void Draw() = 0;
};