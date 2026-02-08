#pragma once

struct InputAction {
	// -1 => no move this frame, otherwise 0..3 (N,E,S,W)
	int moveDir = -1;
	bool shoot = false;
};

class InputMap {
	public:
		static InputAction PollAction();
};
