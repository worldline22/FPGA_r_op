# This is our ISM project

self.data_cls = DataCollections(
            params, placedb, dtype=self.dtype, device=self.device
        )

pos_xyz = loc_xyz[
                self.data_cls.movable_range[0] : self.data_cls.movable_range[1]
            ]

self.movable_range = placedb.movableRange()
