function make_button(x, y, layer, id, callback)
	local button = oak.es:create_entity(layer, "gui_button", require("gui/button"))
	button:set_transform({ position = { x = x, y = y } })
	button.callback = callback
	local icon = oak.es:create_entity(layer, {})
	icon:add_transform({ position = { x = x, y = y, z = 2.0 }})
	icon:add_sprite({ sprite = hash("spr_tool_bar_icons"), animframe_x = id })

	return button, icon
end

local tool_bar = {
	on_create = function(self)
		self.children = {}
		self.buttons = {}

		local tile_editor = oak.es:create_entity(self:layer(), "tile_editor", require("gui/tile_editor"))
		local b1, i1 = make_button(0, 0, self:layer(), 0, function(button, active)
			if active == 1 then
				for k, v in pairs(self.buttons) do
					if v ~= button then
						v:set_active(0)
					end
				end
			end
			tile_editor:toggle_active(active)
		end)

		table.insert(self.children, b1)
		table.insert(self.children, i1)
		table.insert(self.buttons, b1)

		local entity_editor = oak.es:create_entity(self:layer(), "entity_editor", require("gui/entity_editor"))
		local b2, i2 = make_button(48, 0, self:layer(), 1, function(button, active)
			if active == 1 then
				for k, v in pairs(self.buttons) do
					if v ~= button then
						v:set_active(0)
					end
				end
			end
		end)

		table.insert(self.children, b2)
		table.insert(self.children, i2)
		table.insert(self.buttons, b2)

		local console = oak.es:create_entity(self:layer(), "console", require("gui/console"))
		local b3, i3 = make_button(96, 0, self:layer(), 2, function(button, active)
			if active == 1 then
				for k, v in pairs(self.buttons) do
					if v ~= button then
						v:set_active(0)
					end
				end
			end
			console:toggle_active(active)
		end)


		table.insert(self.children, b3)
		table.insert(self.children, i3)
		table.insert(self.buttons, b3)

		local debug = oak.es:create_entity(self:layer(), "debug", require("gui/debug_menu"))
		local b4, i4 = make_button(144, 0, self:layer(), 3, function(button, active)
			if active == 1 then
				for k, v in pairs(self.buttons) do
					if v ~= button then
						v:set_active(0)
					end
				end
			end
		end)


		table.insert(self.children, b4)
		table.insert(self.children, i4)
		table.insert(self.buttons, b4)
	end,

	on_activate = function(self)
		for k, v in pairs(self.children) do
			v:activate()
		end
	end,
	on_deactivate = function(self)
		for k, v in pairs(self.children) do
			v:deactivate()
		end
	end
}


return tool_bar