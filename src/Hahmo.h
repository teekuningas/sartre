// Moi.

class Hahmo
{
private:
	GLfloat x;
	GLfloat y;
	GLfloat vy;
	GLfloat alkupy;
	GLfloat vx;
public:
	SDL_Surface *image[3];
	int hahmo;
	bool hyppy;
	void MuutaX(GLfloat xpar)
	{
		x=xpar;
	}
	void MuutaY(GLfloat ypar)
	{
		y=ypar;
	}
	void MuutaVY(GLfloat vypar)
	{
		vy=vypar;
	}
	void MuutaVX(GLfloat vxpar)
	{
		vx=vxpar;
	}
	void MuutaAlkupY(GLfloat alkupypar)
	{
		alkupy=alkupypar;
	}
	GLfloat LueX()
	{
		return x;
	}
	GLfloat LueY()
	{
		return y;
	}
	GLfloat LueVY()
	{
		return vy;
	}
	GLfloat LueVX()
	{
		return vx;
	}
	GLfloat LueAlkupY()
	{
		return alkupy;
	}



};

class Tausta
{
private:
	GLfloat x;
	GLfloat y;
public:
	SDL_Surface *image;
	void MuutaX(GLfloat xpar)
	{
		x=xpar;
	}
	void MuutaY(GLfloat ypar)
	{
		y=ypar;
	}
	GLfloat LueX()
	{
		return x;
	}
	GLfloat LueY()
	{
		return y;
	}


};