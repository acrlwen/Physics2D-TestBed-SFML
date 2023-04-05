#ifndef PHYSICS2D_SCENES_STACKING_H
#define PHYSICS2D_SCENES_STACKING_H
#include "frame.h"
namespace Physics2D
{
	class StackingFrame : public Frame
	{
	public:
		StackingFrame(PhysicsWorld* world, ContactMaintainer* maintainer,
			Tree* tree, UniformGrid* grid, Camera* camera) : Frame("Stacking", world, maintainer, tree, grid, camera)
		{

		}
		void load() override
		{
			block.set(200, 1.0f);
			edge.set({ -100, 0 }, { 100, 0 });
			rectangle.set(1.0f, 1.0f);

			Body* ground;

			ground = m_world->createBody();
			ground->setShape(&edge);
			ground->position().set({ 0.0, 0.0 });
			ground->setMass(Constant::Max);
			ground->setType(Body::BodyType::Static);
			ground->setFriction(0.3f);
			m_tree->insert(ground);

			//m_grid->insert(ground);

			real offset = 0.0f;
			real max = 25.0;
			for (real j = 0; j < max; j += 1.0f)
			{
				for (real i = 0.0; i < max - j; i += 1.0f)
				{
					Body* body = m_world->createBody();
					body->position().set({ -10.0f + i * 1.05f + offset, j * 1.05f + 1.5f });
					body->setShape(&rectangle);
					body->rotation() = 0;
					body->setMass(1.0f);
					body->setType(Body::BodyType::Dynamic);
					body->setFriction(0.2f);
					body->setRestitution(0.0f);
					m_tree->insert(body);
					//m_grid->insert(body);
				}
				offset += 0.5f;
			}
		}

	private:
		Rectangle rectangle;
		Edge edge;
		Rectangle block;

	};
}
#endif